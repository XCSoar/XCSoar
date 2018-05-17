/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Request.hpp"
#include "Handler.hpp"
#include "Operation/Operation.hpp"

#include <stdint.h>
#include <string.h>

class CancelDownloadToBuffer {};

class DownloadToBufferHandler final : public Net::ResponseHandler {
  uint8_t *buffer;
  const size_t max_size;

  size_t received = 0;

  OperationEnvironment &env;

public:
  DownloadToBufferHandler(void *_buffer, size_t _max_size,
                          OperationEnvironment &_env)
    :buffer((uint8_t *)_buffer), max_size(_max_size), env(_env) {}

  size_t GetReceived() const {
    return received;
  }

  void ResponseReceived(int64_t content_length) override {
    if (content_length > 0)
      env.SetProgressRange(content_length);
  };

  void DataReceived(const void *data, size_t length) override {
    size_t remaining = max_size - received;
    if (remaining == 0 || env.IsCancelled())
      throw CancelDownloadToBuffer();

    if (length > remaining)
      length = remaining;

    memcpy(buffer + received, data, length);
    received += length;

    env.SetProgressRange(received);
  };
};

size_t
Net::DownloadToBuffer(Session &session, const char *url,
                      const char *username, const char *password,
                      void *_buffer, size_t max_length,
                      OperationEnvironment &env)
{
  DownloadToBufferHandler handler(_buffer, max_length, env);
  Request request(session, handler, url);
  if (username != nullptr)
    request.SetBasicAuth(username, password);

  try {
    request.Send(10000);
  } catch (CancelDownloadToBuffer) {
    if (env.IsCancelled())
      return -1;
  }

  return handler.GetReceived();
}

void
Net::DownloadToBufferJob::Run(OperationEnvironment &env)
{
  length = DownloadToBuffer(session, url, username, password,
                            buffer, max_length, env);
}
