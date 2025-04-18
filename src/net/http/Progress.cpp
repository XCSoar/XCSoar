// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Progress.hpp"
#include "lib/curl/Easy.hxx"
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
