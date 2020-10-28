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

#include "ToFile.hpp"
#include "Request.hpp"
#include "Handler.hpp"
#include "Operation/Operation.hpp"
#include "io/FileOutputStream.hxx"
#include "Crypto/SHA256.hxx"

#include <cassert>

class DownloadToFileHandler final : public Net::ResponseHandler {
  OutputStream &out;

  SHA256State sha256;

  size_t received = 0;

  OperationEnvironment &env;

  std::exception_ptr error;

  const bool do_sha256;

public:
  DownloadToFileHandler(OutputStream &_out, bool _do_sha256,
                        OperationEnvironment &_env) noexcept
    :out(_out), env(_env), do_sha256(_do_sha256)
  {
  }

  void CheckError() const {
    if (error)
      std::rethrow_exception(error);
  }

  auto GetSHA256() noexcept {
    assert(do_sha256);

    return sha256.Final();
  }

  bool ResponseReceived(int64_t content_length) noexcept override {
    if (content_length > 0)
      env.SetProgressRange(content_length);
    return true;
  };

  bool DataReceived(const void *data, size_t length) noexcept override {
    if (do_sha256)
      sha256.Update({data, length});

    try {
      out.Write(data, length);
    } catch (...) {
      error = std::current_exception();
      return false;
    }

    received += length;

    env.SetProgressPosition(received);
    return true;
  }
};

static void
DownloadToFile(Net::Session &session, const char *url,
               const char *username, const char *password,
               OutputStream &out, std::array<std::byte, 32> *sha256,
               OperationEnvironment &env)
{
  assert(url != nullptr);

  DownloadToFileHandler handler(out, sha256 != nullptr, env);
  Net::Request request(session, handler, url);
  if (username != nullptr)
    request.SetBasicAuth(username, password);

  try {
    request.Send(10000);
  } catch (...) {
    if (env.IsCancelled())
      /* cancelled by user, not an error: ignore the CURL error */
      return;

    /* see if a pending exception needs to be rethrown */
    handler.CheckError();
    /* no - rethrow the original exception we just caught here */
    throw;
  }

  if (sha256 != nullptr)
    *sha256 = handler.GetSHA256();
}

void
Net::DownloadToFile(Session &session, const char *url,
                    const char *username, const char *password,
                    Path path, std::array<std::byte, 32> *sha256,
                    OperationEnvironment &env)
{
  assert(url != nullptr);
  assert(path != nullptr);

  FileOutputStream file(path);
  ::DownloadToFile(session, url, username, password,
                   file, sha256, env);
  file.Commit();
}

void
Net::DownloadToFileJob::Run(OperationEnvironment &env)
{
  DownloadToFile(session, url, username, password, path, &sha256, env);
}
