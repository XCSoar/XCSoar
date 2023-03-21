// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightInfo.hpp"
#include "time/BrokenDateTime.hpp"

std::chrono::system_clock::duration
FlightInfo::Duration() const noexcept
{
  if (!date.IsPlausible() ||
      !start_time.IsPlausible() || !end_time.IsPlausible())
    return std::chrono::seconds{-1};

  auto duration = BrokenDateTime(date, end_time) - BrokenDateTime(date, start_time);

  // adjust for possible date advance between start and end (add one day)
  if (duration.count() < 0)
    duration += std::chrono::hours(24);

  // if still not a plausible duration return invalid duration
  if (duration.count() < 0 || duration > std::chrono::hours(14))
    return std::chrono::seconds{-1};

  return duration;
}
