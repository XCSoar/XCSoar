// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AppKey.hpp"

namespace LiveTrack24 {

/* issued by LiveTrack24 for the application name "XCSoar".  The
   secret ships inside every build, so LiveTrack24 treats it as an
   application identifier, not as a secret; distribution builds use
   the same key. */
static constexpr std::string_view APP_KEY = "PHE49Y";
static constexpr std::string_view APP_SECRET =
  "TET4PBJOWMpYY5eVYayfk6KH3eTlFPXZ3PXOBlmH9sEKI4PutWsez1FvhygZyBw";

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
