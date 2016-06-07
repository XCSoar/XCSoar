/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "BrokenDate.hpp"
#include "BrokenDateTime.hpp"
#include "DateUtil.hpp"

#include <assert.h>

BrokenDate
BrokenDate::TodayUTC()
{
  return BrokenDateTime::NowUTC();
}

void
BrokenDate::IncrementDay()
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
BrokenDate::DecrementDay()
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
BrokenDate::DaysSince(const BrokenDate &other) const
{
  constexpr int SECONDS_PER_DAY = 24 * 60 * 60;

  constexpr BrokenTime midnight = BrokenTime::Midnight();
  const int64_t a = BrokenDateTime(*this, midnight).ToUnixTimeUTC();
  const int64_t b = BrokenDateTime(other, midnight).ToUnixTimeUTC();
  const int64_t delta = a - b;
  return int(delta / SECONDS_PER_DAY);
}
