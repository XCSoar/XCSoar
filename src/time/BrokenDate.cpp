// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BrokenDate.hpp"
#include "BrokenDateTime.hpp"
#include "Calendar.hxx"

#include <cassert>

BrokenDate
BrokenDate::TodayUTC() noexcept
{
  return BrokenDateTime::NowUTC();
}

void
BrokenDate::IncrementDay() noexcept
{
  assert(IsPlausible());

  const unsigned max_day = DaysInMonth(month, year);

  ++day;

  if (day > max_day) {
    /* roll over to next month */
    day = 1;
    ++month;
    if (month > 12) {
      /* roll over to next year */
      month = 1;
      ++year;
    }
  }

  if (day_of_week >= 0) {
    ++day_of_week;
    if (day_of_week >= 7)
      day_of_week = 0;
  }
}

void
BrokenDate::DecrementDay() noexcept
{
  assert(IsPlausible());

  --day;

  if (day < 1) {
    --month;

    if (month < 1) {
      --year;
      month = 12;
    }

    day = DaysInMonth(month, year);
  }
}

int
BrokenDate::DaysSince(const BrokenDate &other) const noexcept
{
  constexpr std::chrono::system_clock::duration one_day = std::chrono::hours(24);

  constexpr BrokenTime midnight = BrokenTime::Midnight();
  const auto a = BrokenDateTime(*this, midnight).ToTimePoint();
  const auto b = BrokenDateTime(other, midnight).ToTimePoint();
  const auto delta = a - b;
  return int(delta / one_day);
}
