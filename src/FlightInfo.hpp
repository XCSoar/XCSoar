// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenDate.hpp"
#include "time/BrokenTime.hpp"

struct FlightInfo {
  BrokenDate date;

  BrokenTime start_time, end_time;

  std::chrono::system_clock::duration Duration() const noexcept;
};

/**
 * Clock plus a duration, both stored as BrokenTime.  Wraps at midnight.
 *
 * The hour field is a uint8_t.  Subtracting 24 from 23 wraps to 255
 * and walks down to 15, so the bound is 24.
 */
constexpr BrokenTime
AddClock(BrokenTime start, BrokenTime duration) noexcept
{
  BrokenTime sum;
  sum.hour = start.hour + duration.hour;
  sum.minute = start.minute + duration.minute;
  sum.second = start.second + duration.second;

  while (sum.second >= 60) {
    sum.second -= 60;
    sum.minute++;
  }

  while (sum.minute >= 60) {
    sum.minute -= 60;
    sum.hour++;
  }

  while (sum.hour >= 24)
    sum.hour -= 24;

  return sum;
}
