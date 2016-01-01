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

#ifndef NET_CURL_MULTI_HPP
#define NET_CURL_MULTI_HPP

#include "Compiler.h"

#include <map>
#include <stdexcept>

#include <curl/curl.h>

namespace Net {
  /**
   * Wrapper for a CURLM object.  This class is not thread-safe.
   */
  class CurlMulti {
    CURLM *multi;

    std::map<const CURL *, CURLcode> results;

  public:
    CurlMulti();
    CurlMulti(const CurlMulti &other) = delete;

    ~CurlMulti();

    CurlMulti &operator=(const CurlMulti &other) = delete;

    bool IsDefined() const {
      return multi != nullptr;
    }

    void Add(CURL *easy) {
      CURLMcode code = curl_multi_add_handle(multi, easy);
      if (code != CURLM_OK)
        throw std::runtime_error(curl_multi_strerror(code));
    }

    void Remove(CURL *easy);

    void FdSet(fd_set *read_fd_set, fd_set *write_fd_set, fd_set *exc_fd_set,
               int *max_fd) const {
      CURLMcode code = curl_multi_fdset(multi, read_fd_set, write_fd_set,
                                        exc_fd_set, max_fd);
      if (code != CURLM_OK)
        throw std::runtime_error(curl_multi_strerror(code));
    }

    gcc_pure
    long GetTimeout() const {
      long timeout;
      return ::curl_multi_timeout(multi, &timeout) == CURLM_OK
          ? timeout
          : -1;
    }

    CURLMcode Perform() {
      int running_handles;
      return ::curl_multi_perform(multi, &running_handles);
    }

    gcc_pure
    CURLcode InfoRead(const CURL *easy);
  };
}

#endif
