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

#include "Progress.hpp"
#include "Easy.hxx"
#include "Operation/ProgressListener.hpp"

#include <limits.h>

namespace Net {

ProgressAdapter::ProgressAdapter(CurlEasy &curl, ProgressListener &_listener)
  :listener(_listener)
{
  curl.SetXferInfoFunction(_Callback, this);
}

inline void
ProgressAdapter::Callback(curl_off_t dltotal, curl_off_t dlnow,
                          curl_off_t ultotal, curl_off_t ulnow) noexcept
{
  /* if we don't know the length, don't use the "now" value either */
  if (dltotal == 0)
    dlnow = 0;
  if (ultotal == 0)
    ulnow = 0;

  const auto range = dltotal + ultotal;
  if (range == 0 || range > UINT_MAX)
    /* no useful information at all (or overflow) */
    return;

  listener.SetProgressRange(range);
  listener.SetProgressPosition(dlnow + ulnow);
}

int
ProgressAdapter::_Callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                           curl_off_t ultotal, curl_off_t ulnow) noexcept
{
  auto &pa = *(ProgressAdapter *)clientp;
  pa.Callback(dltotal, dlnow, ultotal, ulnow);
  return 0;
}

} // namespace Net
