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

#ifndef NET_TO_FILE_HPP
#define NET_TO_FILE_HPP

#include "Job/Job.hpp"
#include "system/Path.hpp"

#include <array>
#include <cstddef> // for std::byte

class OperationEnvironment;
class CurlGlobal;

namespace Net {

/**
 * Download a URL into the specified file.
 *
 * Throws on error.
 *
 * @param md5_digest an optional buffer with at least 33 characters
 * which will contain the hex MD5 digest after returning
 */
void DownloadToFile(CurlGlobal &curl, const char *url,
                    const char *username, const char *password,
                    Path path, std::array<std::byte, 32> *sha256,
                    OperationEnvironment &env);

static inline void
DownloadToFile(CurlGlobal &curl, const char *url,
               Path path, std::array<std::byte, 32> *sha256,
               OperationEnvironment &env)
{
  DownloadToFile(curl, url, nullptr, nullptr,
                 path, sha256, env);
}

class DownloadToFileJob : public Job {
  CurlGlobal &curl;
  const char *url;
  const char *username = nullptr, *password = nullptr;
  const Path path;
  std::array<std::byte, 32> sha256;

public:
  DownloadToFileJob(CurlGlobal &_curl, const char *_url, Path _path)
    :curl(_curl), url(_url), path(_path) {}

  void SetBasicAuth(const char *_username, const char *_password) {
    username = _username;
    password = _password;
  }

  const auto &GetSHA256() const {
    return sha256;
  }

  virtual void Run(OperationEnvironment &env);
};

} // namespace Net

#endif
