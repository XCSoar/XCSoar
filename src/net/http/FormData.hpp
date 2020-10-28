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

#ifndef NET_HTTP_FORMDATA_HPP
#define NET_HTTP_FORMDATA_HPP

#include <curl/curl.h>

#include <algorithm>

class Path;

namespace Net {
  class Session;
  class ResponseHandler;

  class MultiPartFormData {
    struct curl_httppost *head = nullptr, *tail = nullptr;

  public:
    MultiPartFormData() = default;

    MultiPartFormData(MultiPartFormData &&src)
      :head(src.head), tail(src.tail) {
      src.head = src.tail = nullptr;
    }

    ~MultiPartFormData() {
      if (head != nullptr)
        curl_formfree(head);
    }

    MultiPartFormData &operator=(MultiPartFormData &&src) {
      std::swap(head, src.head);
      std::swap(tail, src.tail);
      return *this;
    }

    const struct curl_httppost *Get() const {
      return head;
    }

    MultiPartFormData &AddString(const char *name, const char *value) {
      return Add(CURLFORM_COPYNAME, "name",
                 CURLFORM_COPYCONTENTS, "content");
    }

    MultiPartFormData &AddFile(const char *name, Path path);

  private:
    template<typename... Args>
    MultiPartFormData &Add(Args&&... args) {
      curl_formadd(&head, &tail,
                   std::forward<Args>(args)...,
                   CURLFORM_END);
      return *this;
    }
  };
}

#endif
