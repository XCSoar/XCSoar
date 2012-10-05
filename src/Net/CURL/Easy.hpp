/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef NET_CURL_EASY_HPP
#define NET_CURL_EASY_HPP

#include "Compiler.h"

#include <curl/curl.h>

#include <assert.h>
#include <stdint.h>

namespace Net {
  /**
   * Wrapper for a CURLM object.  This class is not thread-safe.
   */
  class CurlEasy {
    CURL *handle;

  public:
    CurlEasy():handle(::curl_easy_init()) {}
    CurlEasy(const CurlEasy &other) = delete;

    ~CurlEasy() {
      if (handle != nullptr)
        ::curl_easy_cleanup(handle);
    }

    CurlEasy &operator=(const CurlEasy &other) = delete;

    bool IsDefined() const {
      return handle != nullptr;
    }

    CURL *GetHandle() {
      assert(IsDefined());

      return handle;
    }

    void Destroy() {
      assert(IsDefined());

      ::curl_easy_cleanup(handle);
      handle = nullptr;
    }

    template<typename T>
    bool SetOption(CURLoption option, T value) {
      assert(IsDefined());

      return ::curl_easy_setopt(handle, option, value) == CURLE_OK;
    }

    bool SetURL(const char *value) {
      return SetOption(CURLOPT_URL, value);
    }

    bool SetUserAgent(const char *value) {
      return SetOption(CURLOPT_USERAGENT, value);
    }

    bool SetFailOnError(bool value) {
      return SetOption(CURLOPT_FAILONERROR, value);
    }

    bool SetWriteFunction(size_t (*function)(char *ptr, size_t size,
                                             size_t nmemb, void *userdata),
                          void *userdata) {
      return SetOption(CURLOPT_WRITEFUNCTION, function) &&
        SetOption(CURLOPT_WRITEDATA, userdata);
    }

    template<typename T>
    bool GetInfo(CURLINFO info, T value_r) const {
      assert(IsDefined());

      return ::curl_easy_getinfo(handle, info, value_r) == CURLE_OK;
    }

    gcc_pure
    int64_t GetContentLength() const {
      double value;
      return GetInfo(CURLINFO_CONTENT_LENGTH_DOWNLOAD, &value) == CURLE_OK
        ? (int64_t)value
        : -1;
    }

    bool Unpause() {
      assert(IsDefined());

      return ::curl_easy_pause(handle, CURLPAUSE_CONT) == CURLE_OK;
    }
  };
}

#endif
