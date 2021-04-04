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

#ifndef NET_TO_BUFFER_HPP
#define NET_TO_BUFFER_HPP

#include "Job/Job.hpp"

#include <cstddef>

class OperationEnvironment;
class CurlGlobal;

namespace Net {

/**
 * Download a URL into the specified buffer.  If the response is too
 * long, it is truncated.
 *
 * @return the number of bytes written
 */
size_t
DownloadToBuffer(CurlGlobal &curl, const char *url,
                 const char *username, const char *password,
                 void *buffer, size_t max_length,
                 OperationEnvironment &env);

static inline size_t
DownloadToBuffer(CurlGlobal &curl, const char *url,
                 void *buffer, size_t max_length,
                 OperationEnvironment &env)
{
  return DownloadToBuffer(curl, url, nullptr, nullptr,
                          buffer, max_length, env);
}

class DownloadToBufferJob : public Job {
  CurlGlobal &curl;
  const char *url;
  const char *username = nullptr, *password = nullptr;
  void *buffer;
  size_t max_length;
  size_t length;

public:
  DownloadToBufferJob(CurlGlobal &_curl, const char *_url,
                      void *_buffer, size_t _max_length)
    :curl(_curl), url(_url),
     buffer(_buffer), max_length(_max_length),
     length(-1) {}

  void SetBasicAuth(const char *_username, const char *_password) {
    username = _username;
    password = _password;
  }

  size_t GetLength() const {
    return length;
  }

  /* virtual methods from class Job */
  void Run(OperationEnvironment &env) override;
};

} // namespace Net

#endif
