/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "ToBuffer.hpp"
#include "Request.hxx"
#include "Handler.hxx"
#include "Operation/Operation.hpp"
#include "thread/Mutex.hxx"
#include "thread/Cond.hxx"
#include "util/NumberParser.hpp"
#include "util/ScopeExit.hxx"

#include <cstdint>
#include <string.h>

class DownloadToBufferHandler final : public CurlResponseHandler {
  uint8_t *buffer;
  const size_t max_size;

  size_t received = 0;

  OperationEnvironment &env;

  Mutex mutex;
  Cond cond;

  std::exception_ptr error;

  bool done = false;

public:
  DownloadToBufferHandler(void *_buffer, size_t _max_size,
                          OperationEnvironment &_env)
    :buffer((uint8_t *)_buffer), max_size(_max_size), env(_env) {}

  size_t GetReceived() const {
    return received;
  }

  void Cancel() noexcept {
    const std::lock_guard<Mutex> lock(mutex);
    done = true;
    cond.notify_one();
  }

  void Wait() noexcept {
    std::unique_lock<Mutex> lock(mutex);
    cond.wait(lock, [this]{ return done; });

    if (error)
      std::rethrow_exception(error);
  }

  /* virtual methods from class CurlResponseHandler */
  void OnHeaders(unsigned status,
                 std::multimap<std::string, std::string> &&headers) override {
    if (auto i = headers.find("content-length"); i != headers.end())
      env.SetProgressRange(ParseUint64(i->second.c_str()));
  }

  void OnData(ConstBuffer<void> data) override {
    size_t remaining = max_size - received;
    if (remaining == 0 || done)
      throw Pause{};

    if (data.size > remaining)
      data.size = remaining;

    memcpy(buffer + received, data.data, data.size);
    received += data.size;

    env.SetProgressRange(received);
  }

  void OnEnd() override {
    const std::lock_guard<Mutex> lock(mutex);
    done = true;
    cond.notify_one();
  }

  void OnError(std::exception_ptr e) noexcept override {
    const std::lock_guard<Mutex> lock(mutex);
    error = std::move(e);
    done = true;
    cond.notify_one();
  }
};

size_t
Net::DownloadToBuffer(CurlGlobal &curl, const char *url,
                      const char *username, const char *password,
                      void *_buffer, size_t max_length,
                      OperationEnvironment &env)
{
  DownloadToBufferHandler handler(_buffer, max_length, env);
  CurlRequest request(curl, url, handler);
  AtScopeExit(&request) { request.StopIndirect(); };

  request.SetFailOnError();

  if (username != nullptr)
    request.SetOption(CURLOPT_USERNAME, username);
  if (password != nullptr)
    request.SetOption(CURLOPT_PASSWORD, password);

  env.SetCancelHandler([&]{
    request.StopIndirect();
    handler.Cancel();
  });

  AtScopeExit(&env) { env.SetCancelHandler({}); };

  request.StartIndirect();
  handler.Wait();

  if (env.IsCancelled())
    return -1;

  return handler.GetReceived();
}

void
Net::DownloadToBufferJob::Run(OperationEnvironment &env)
{
  length = DownloadToBuffer(curl, url, username, password,
                            buffer, max_length, env);
}
