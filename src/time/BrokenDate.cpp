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

BrokenDate
BrokenDate::FromJulianDate(uint32_t julian_date) noexcept
{
  // Convert Julian date BrokenDate
  BrokenDate bd;
  int a = julian_date + 32044;
  int b = (4 * a + 3) / 146097;
  int c = a - (146097 * b) / 4;
  int d = (4 * c + 3) / 1461;
  int e = c - (1461 * d) / 4;
  int m = (5 * e + 2) / 153;

  bd.day = e - (153 * m + 2) / 5 + 1;
  bd.month = m + 3 - 12 * (m / 10);
  bd.year = 100 * b + d - 4800 + (m / 10);

  // Calculate day-of-week using Zeller's Congruence algorithm
  m = bd.month;
  int y = bd.year;
  if (m < 3) {
    m += 12;
    y -= 1;
  }

  int k = y % 100;
  int j = y / 100;

  int dayOfWeek =
    (bd.day + (13 * (m + 1)) / 5 + k + (k / 4) + (j / 4) - 2 * j) % 7;

  // Adjust the result to match the day of the week format (0=Saturday)
  bd.day_of_week = (dayOfWeek + 6) % 7;

  return bd;
}
