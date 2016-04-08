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

#ifndef NET_CURL_EASY_HPP
#define NET_CURL_EASY_HPP

#include "Compiler.h"

#include <curl/curl.h>

#include <new>
#include <stdexcept>

#include <assert.h>
#include <stdint.h>

namespace Net {
  /**
   * Wrapper for a CURLM object.  This class is not thread-safe.
   */
  class CurlEasy {
    CURL *handle;

  public:
    CurlEasy():handle(::curl_easy_init()) {
      if (handle == nullptr)
        throw std::bad_alloc();
    }

    CurlEasy(const CurlEasy &other) = delete;

    ~CurlEasy() {
      ::curl_easy_cleanup(handle);
    }

    CurlEasy &operator=(const CurlEasy &other) = delete;

    CURL *GetHandle() {
      return handle;
    }

    void Destroy() {
      ::curl_easy_cleanup(handle);
      handle = nullptr;
    }

    template<typename T>
    void SetOption(CURLoption option, T value) {
      CURLcode code = ::curl_easy_setopt(handle, option, value);
      if (code != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(code));
    }

    void SetURL(const char *value) {
      SetOption(CURLOPT_URL, value);
    }

    void SetUserAgent(const char *value) {
      SetOption(CURLOPT_USERAGENT, value);
    }

    void SetHeaders(struct curl_slist *headers) {
      SetOption(CURLOPT_HTTPHEADER, headers);
    }

    void SetBasicAuth(const char *userpwd) {
      SetOption(CURLOPT_USERPWD, userpwd);
    }

    void SetFailOnError(bool value) {
      SetOption(CURLOPT_FAILONERROR, value);
    }

    void SetWriteFunction(size_t (*function)(char *ptr, size_t size,
                                             size_t nmemb, void *userdata),
                          void *userdata) {
      SetOption(CURLOPT_WRITEFUNCTION, function);
      SetOption(CURLOPT_WRITEDATA, userdata);
    }

    void SetHttpPost(const struct curl_httppost *post) {
      SetOption(CURLOPT_HTTPPOST, post);
    }

    template<typename T>
    bool GetInfo(CURLINFO info, T value_r) const {
      return ::curl_easy_getinfo(handle, info, value_r) == CURLE_OK;
    }

    gcc_pure
    int64_t GetContentLength() const {
      double value;
      return GetInfo(CURLINFO_CONTENT_LENGTH_DOWNLOAD, &value)
        ? (int64_t)value
        : -1;
    }

    bool Unpause() {
      return ::curl_easy_pause(handle, CURLPAUSE_CONT) == CURLE_OK;
    }
  };
}

#endif
