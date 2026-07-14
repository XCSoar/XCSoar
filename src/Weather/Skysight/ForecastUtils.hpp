// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Layers.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

namespace SkySight {

[[nodiscard]] inline bool
IsFullDayForecastLayerId(std::string_view id) noexcept
{
  return id == "pfdtot";
}

[[nodiscard]] inline bool
IsFullDayForecastLayer(const Layer &layer) noexcept
{
  return IsFullDayForecastLayerId(layer.id);
}

[[nodiscard]] inline time_t
GetForecastDayBucket(time_t forecast_time) noexcept
{
  return forecast_time > 0 ? forecast_time / (24 * 60 * 60) : 0;
}

[[nodiscard]] inline time_t
GetForecastDayStart(time_t forecast_time) noexcept
{
  return GetForecastDayBucket(forecast_time) * 24 * 60 * 60;
}

[[nodiscard]] inline bool
IsSameForecastDay(time_t left, time_t right) noexcept
{
  return left > 0 && right > 0 &&
    GetForecastDayBucket(left) == GetForecastDayBucket(right);
}

[[nodiscard]] inline std::vector<const ForecastDatafile *>
GetForecastPreloadDatafiles(const Layer &layer, time_t now)
{
  const time_t day_start = GetForecastDayStart(now);
  std::vector<const ForecastDatafile *> result;
  result.reserve(layer.forecast_datafiles.size());

  for (const auto &datafile : layer.forecast_datafiles)
    if (datafile.time >= day_start && !datafile.link.empty())
      result.push_back(&datafile);

  std::sort(result.begin(), result.end(), [](const auto *a, const auto *b) {
    return a->time < b->time;
  });

  if (IsFullDayForecastLayer(layer))
    result.erase(std::unique(result.begin(), result.end(),
                             [](const auto *a, const auto *b) {
                               return IsSameForecastDay(a->time, b->time);
                             }),
                 result.end());

  return result;
}

} // namespace SkySight
