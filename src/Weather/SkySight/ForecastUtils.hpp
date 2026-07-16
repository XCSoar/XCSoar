// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Layers.hpp"

#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>
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

template<typename Range, typename GetTime>
[[nodiscard]] inline time_t
ChooseClosestForecastTime(const Range &values, GetTime get_time, time_t now)
  noexcept(noexcept(get_time(*std::begin(values))))
{
  time_t latest_past = 0;
  time_t earliest_future = 0;
  for (const auto &value : values) {
    const auto time = get_time(value);
    if (time <= 0)
      continue;

    if (time <= now)
      latest_past = std::max(latest_past, time);
    else if (earliest_future == 0 || time < earliest_future)
      earliest_future = time;
  }

  return latest_past != 0 ? latest_past : earliest_future;
}

template<typename Range, typename GetTime>
[[nodiscard]] inline time_t
ChooseClosestForecastTime(const Range &values, GetTime get_time)
  noexcept(noexcept(get_time(*std::begin(values))))
{
  return ChooseClosestForecastTime(values, get_time, std::time(nullptr));
}

/**
 * Merge display-ready cache entries into the canonical forecast metadata.
 * Existing download links and a still-valid selection are preserved.
 */
inline void
MergeCachedForecastTimes(Layer &layer, const std::vector<time_t> &cached_times,
                         time_t now)
{
  for (const auto time : cached_times) {
    if (time <= 0 || layer.FindDatafile(time) != nullptr)
      continue;

    layer.forecast_datafiles.emplace_back(time, std::string{});
  }

  std::sort(layer.forecast_datafiles.begin(),
            layer.forecast_datafiles.end(),
            [](const auto &left, const auto &right) {
              return left.time > right.time;
            });

  if (layer.forecast_datafiles.empty())
    return;

  layer.from = layer.forecast_datafiles.back().time;
  layer.to = layer.forecast_datafiles.front().time;

  if (layer.forecast_time <= 0 ||
      layer.FindDatafile(layer.forecast_time) == nullptr)
    layer.forecast_time = ChooseClosestForecastTime(
      layer.forecast_datafiles,
      [](const auto &datafile) noexcept {
        return datafile.time;
      }, now);
}

template<typename T, typename GetTime>
inline void
CollapseFullDayForecastValues(bool full_day, std::vector<T> &values,
                              GetTime get_time)
{
  if (!full_day || values.empty())
    return;

  std::vector<T> collapsed;
  collapsed.reserve(values.size());

  for (auto &value : values) {
    if (!collapsed.empty() &&
        IsSameForecastDay(get_time(collapsed.back()), get_time(value)))
      continue;

    collapsed.push_back(std::move(value));
  }

  values = std::move(collapsed);
}

inline void
CollapseFullDayForecastTimes(std::string_view layer_id,
                             std::vector<time_t> &times)
{
  CollapseFullDayForecastValues(IsFullDayForecastLayerId(layer_id), times,
                                [](time_t time) noexcept {
                                  return time;
                                });
}

inline void
CollapseFullDayForecastDatafiles(const Layer &layer,
                                 std::vector<ForecastDatafile> &forecast_datafiles)
{
  CollapseFullDayForecastValues(IsFullDayForecastLayer(layer),
                                forecast_datafiles,
                                [](const auto &datafile) noexcept {
                                  return datafile.time;
                                });
}

/**
 * Returns pointers into Layer::forecast_datafiles.  They remain valid only
 * while the layer exists and its forecast_datafiles vector is not mutated.
 */
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
