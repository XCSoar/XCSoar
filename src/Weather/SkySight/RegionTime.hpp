// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/RoughTime.hpp"

#include <ctime>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
#include <chrono>
#endif

#ifdef __APPLE__
#include <time.h>
#endif

namespace SkySight {

/**
 * SkySight keeps forecast/live instants as UTC internally.  Convert to the
 * region's civil offset only at edges (URL path segments, UI labels).
 *
 * @p tz_name is the IANA id from the regions API (e.g. Europe/Warsaw).
 * Returns UTC (zero) when the zone cannot be resolved.
 */
namespace region_time_detail {

[[nodiscard]] inline std::optional<RoughTimeDelta>
TryChronoOffset(std::string_view tz_name, time_t utc_time) noexcept
{
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
  try {
    const auto *zone = std::chrono::locate_zone(std::string{tz_name});
    const auto tp = std::chrono::system_clock::from_time_t(utc_time);
    const auto offset = zone->get_info(tp).offset;
    return RoughTimeDelta::FromSeconds(
      std::chrono::duration_cast<std::chrono::seconds>(offset).count());
  } catch (...) {
    return std::nullopt;
  }
#else
  (void)tz_name;
  (void)utc_time;
  return std::nullopt;
#endif
}

#ifdef __APPLE__
[[nodiscard]] inline std::optional<RoughTimeDelta>
TryAppleOffset(std::string_view tz_name, time_t utc_time) noexcept
{
  const std::string name{tz_name};
  timezone_t zone = tzalloc(name.c_str());
  if (zone == nullptr)
    return std::nullopt;

  struct tm tm {};
  const auto *local = localtime_rz(zone, &utc_time, &tm);
  tzfree(zone);
  if (local == nullptr)
    return std::nullopt;

  return RoughTimeDelta::FromSeconds(local->tm_gmtoff);
}
#endif

#ifndef _WIN32
/**
 * Last-resort portable path: temporarily set TZ for localtime_r.
 * Serialized because TZ is process-global.
 */
[[nodiscard]] inline std::optional<RoughTimeDelta>
TryPosixTzOffset(std::string_view tz_name, time_t utc_time) noexcept
{
  static std::mutex mutex;
  const std::lock_guard lock{mutex};

  const char *previous = getenv("TZ");
  std::string previous_value;
  const bool had_previous = previous != nullptr;
  if (had_previous)
    previous_value = previous;

  const std::string name{tz_name};
  if (setenv("TZ", name.c_str(), 1) != 0)
    return std::nullopt;
  tzset();

  struct tm tm {};
  const auto *local = localtime_r(&utc_time, &tm);

  if (had_previous)
    setenv("TZ", previous_value.c_str(), 1);
  else
    unsetenv("TZ");
  tzset();

  if (local == nullptr)
    return std::nullopt;

  return RoughTimeDelta::FromSeconds(tm.tm_gmtoff);
}
#endif

} // namespace region_time_detail

[[nodiscard]] inline RoughTimeDelta
GetRegionUtcOffset(std::string_view tz_name, time_t utc_time) noexcept
{
  if (tz_name.empty() || utc_time <= 0)
    return RoughTimeDelta{};

  if (const auto offset =
        region_time_detail::TryChronoOffset(tz_name, utc_time);
      offset)
    return *offset;

#ifdef __APPLE__
  if (const auto offset =
        region_time_detail::TryAppleOffset(tz_name, utc_time);
      offset)
    return *offset;
#endif

#ifndef _WIN32
  if (const auto offset =
        region_time_detail::TryPosixTzOffset(tz_name, utc_time);
      offset)
    return *offset;
#endif

  return RoughTimeDelta{};
}

} // namespace SkySight
