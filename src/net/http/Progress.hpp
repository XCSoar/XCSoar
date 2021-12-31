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

#ifndef NET_HTTP_PROGRESS_HPP
#define NET_HTTP_PROGRESS_HPP

#include <curl/system.h>

class CurlEasy;
class ProgressListener;

namespace Net {

/**
 * This class reports upload and download progress to an
 * #OperationEnvironment instance.
 */
class ProgressAdapter {
  ProgressListener &listener;

public:
  /**
   * Register as "xferinfo" callback with the given #CurlEasy and keep
   * reporting progress to the #OperationEnvironment instance.
   */
  ProgressAdapter(CurlEasy &curl, ProgressListener &listener);

private:
  void Callback(curl_off_t dltotal, curl_off_t dlnow,
                curl_off_t ultotal, curl_off_t ulnow) noexcept;

  static int _Callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                       curl_off_t ultotal, curl_off_t ulnow) noexcept;
};

} // namespace Net

#endif
