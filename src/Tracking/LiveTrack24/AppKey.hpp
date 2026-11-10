// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string_view>

namespace LiveTrack24 {

/**
 * The AppKey issued by LiveTrack24 for XCSoar.  The protocol sends
 * it, and hashes the AppSecret into every request, so neither can be
 * kept secret; they are plain constants.
 */
[[gnu::const]]
std::string_view
GetAppKey() noexcept;

[[gnu::const]]
std::string_view
GetAppSecret() noexcept;

/** Without a key, XCSoar does not upload to LiveTrack24. */
[[gnu::const]]
inline bool
HasAppKey() noexcept
{
  return !GetAppKey().empty() && !GetAppSecret().empty();
}

} // namespace LiveTrack24
