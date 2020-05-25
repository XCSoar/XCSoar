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
#include "OS/FileUtil.hpp"
#include "Crypto/SHA256.hxx"

#include <cassert>
#include <stdio.h>

class CancelDownloadToFile {};

class DownloadToFileHandler final : public Net::ResponseHandler {
  FILE *file;

  SHA256State sha256;

  size_t received = 0;

  OperationEnvironment &env;

  const bool do_sha256;

public:
  DownloadToFileHandler(FILE *_file, bool _do_sha256,
                        OperationEnvironment &_env) noexcept
    :file(_file), env(_env), do_sha256(_do_sha256)
  {
  }

  auto GetSHA256() noexcept {
    assert(do_sha256);

    return sha256.Final();
  }

  void ResponseReceived(int64_t content_length) override {
    if (content_length > 0)
      env.SetProgressRange(content_length);
  };

  void DataReceived(const void *data, size_t length) override {
    if (do_sha256)
      sha256.Update({data, length});

    size_t written = fwrite(data, 1, length, file);
    if (written != (size_t)length)
      throw CancelDownloadToFile();

    received += length;

    env.SetProgressRange(received);
  };
};

static bool
DownloadToFile(Net::Session &session, const char *url,
               const char *username, const char *password,
               FILE *file, std::array<std::byte, 32> *sha256,
               OperationEnvironment &env)
{
  assert(url != nullptr);
  assert(file != nullptr);

  DownloadToFileHandler handler(file, sha256 != nullptr, env);
  Net::Request request(session, handler, url);
  if (username != nullptr)
    request.SetBasicAuth(username, password);

  try {
    request.Send(10000);
  } catch (CancelDownloadToFile) {
    return false;
  }

  if (sha256 != nullptr)
    *sha256 = handler.GetSHA256();

  return true;
}

bool
Net::DownloadToFile(Session &session, const char *url,
                    const char *username, const char *password,
                    Path path, std::array<std::byte, 32> *sha256,
                    OperationEnvironment &env)
{
  assert(url != nullptr);
  assert(path != nullptr);

  /* make sure we create a new file */
  if (!File::Delete(path) && File::ExistsAny(path))
    /* failed to delete the old file */
    return false;

  /* now create the new file */
  FILE *file = _tfopen(path.c_str(), _T("wb"));
  if (file == nullptr)
    return false;

  bool success = ::DownloadToFile(session, url, username, password,
                                  file, sha256, env);
  success &= fclose(file) == 0;

  if (!success)
    /* delete the partial file on error */
    File::Delete(path);

  return success;
}

void
Net::DownloadToFileJob::Run(OperationEnvironment &env)
{
  success = DownloadToFile(session, url, username, password,
                           path, &sha256, env);
}
