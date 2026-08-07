// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/RoughTime.hpp"

#include <ctime>
#include <string>
#include <string_view>

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
#include <chrono>
#endif

namespace SkySight {

/**
 * Convert a UTC instant to the UTC offset that applies in @p tz_name at that
 * moment (IANA id from the SkySight regions catalog).
 *
 * Falls back to @p fallback when the zone cannot be resolved (missing tzdata,
 * unknown id, or no C++20 timezone support on the target).
 */
[[nodiscard]] inline RoughTimeDelta
GetRegionUtcOffset(std::string_view tz_name, time_t utc_time,
                   RoughTimeDelta fallback) noexcept
{
  if (tz_name.empty() || utc_time <= 0)
    return fallback;

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
  try {
    const auto *zone = std::chrono::locate_zone(std::string{tz_name});
    const auto tp = std::chrono::system_clock::from_time_t(utc_time);
    const auto offset = zone->get_info(tp).offset;
    return RoughTimeDelta::FromSeconds(
      std::chrono::duration_cast<std::chrono::seconds>(offset).count());
  } catch (...) {
    return fallback;
  }
#else
  (void)tz_name;
  (void)utc_time;
  return fallback;
#endif
}

} // namespace SkySight
