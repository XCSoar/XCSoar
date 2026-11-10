// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AppKey.hpp"

namespace LiveTrack24 {

/* issued by LiveTrack24 for the application name "XCSoar"; empty
   until it arrives (#3467) */
static constexpr std::string_view APP_KEY = "";
static constexpr std::string_view APP_SECRET = "";

std::string_view
GetAppKey() noexcept
{
  return APP_KEY;
}

std::string_view
GetAppSecret() noexcept
{
  return APP_SECRET;
}

} // namespace LiveTrack24
