// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermForecastTime.hpp"
#include "Interface.hpp"

namespace XCTherm {

UtcTimeParts
GetUtcTimeParts() noexcept
{
  UtcTimeParts utc;
  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible()) {
    utc.hour = basic.date_time_utc.hour;
    utc.minute = basic.date_time_utc.minute;
  }
  return utc;
}

int
PickAutoTimeIndex(const std::vector<unsigned> &cached_hours,
                  unsigned utc_h, unsigned utc_min) noexcept
{
  if (cached_hours.empty())
    return -1;

  const unsigned target = (utc_min >= 45) ? (utc_h + 1) % 24 : utc_h;

  int best_future = -1;
  unsigned best_future_dist = 25;
  int best_past = -1;
  unsigned best_past_dist = 25;

  for (size_t i = 0; i < cached_hours.size(); ++i) {
    const unsigned fwd = (cached_hours[i] + 24 - target) % 24;
    if (fwd <= 12) {
      if (fwd < best_future_dist) {
        best_future_dist = fwd;
        best_future = int(i);
      }
    } else {
      const unsigned back = 24 - fwd;
      if (back < best_past_dist) {
        best_past_dist = back;
        best_past = int(i);
      }
    }
  }

  return best_future >= 0 ? best_future : best_past;
}

unsigned
ForecastHourAt(const std::vector<unsigned> &cached_hours,
             unsigned time_index,
             const std::vector<unsigned> &available_hours,
             unsigned fallback) noexcept
{
  if (time_index < cached_hours.size())
    return cached_hours[time_index];
  if (!available_hours.empty())
    return available_hours[0];
  return fallback;
}

} // namespace XCTherm
