// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BrokenTime.hpp"

#include <cassert>

BrokenTime
BrokenTime::FromSecondOfDay(unsigned second_of_day) noexcept
{
  assert(second_of_day < 3600u * 24u);

  unsigned hour = second_of_day / 3600u;
  unsigned second_of_hour = second_of_day % 3600u;
  return BrokenTime(hour, second_of_hour / 60u, second_of_hour % 60u);
}

BrokenTime
BrokenTime::FromSecondOfDayChecked(unsigned second_of_day) noexcept
{
  return FromSecondOfDay(second_of_day % (3600u * 24u));
}

BrokenTime
BrokenTime::FromMinuteOfDay(unsigned minute_of_day) noexcept
{
  assert(minute_of_day < 60u * 24u);

  return BrokenTime(minute_of_day / 60u, minute_of_day % 60u);
}

BrokenTime
BrokenTime::FromMinuteOfDayChecked(unsigned minute_of_day) noexcept
{
  return FromMinuteOfDay(minute_of_day % (60u * 24u));
}

BrokenTime
BrokenTime::operator+(std::chrono::seconds delta) const noexcept
{
  assert(IsPlausible());

  delta += DurationSinceMidnight();
  while (delta.count() < 0)
    delta += std::chrono::hours(24);

  return FromSecondOfDayChecked(std::chrono::seconds{delta}.count());
}
