/* Copyright_License {

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

#include "Time/BrokenDateTime.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

static void
TestDate()
{
  ok1(BrokenDate(2010, 1, 2).year == 2010);
  ok1(BrokenDate(2010, 1, 2).month == 1);
  ok1(BrokenDate(2010, 1, 2).day == 2);

  ok1(BrokenDate(2010, 1, 1) == BrokenDate(2010, 1, 1));
  ok1(!(BrokenDate(2010, 1, 1) == BrokenDate(2010, 1, 2)));
  ok1(!(BrokenDate(2010, 1, 1) == BrokenDate(2010, 2, 1)));
  ok1(!(BrokenDate(2010, 1, 1) == BrokenDate(2011, 1, 1)));

  ok1(!(BrokenDate(2010, 1, 1) > BrokenDate(2010, 1, 1)));
  ok1(!(BrokenDate(2010, 1, 1) > BrokenDate(2010, 1, 2)));
  ok1(BrokenDate(2010, 1, 2) > BrokenDate(2010, 1, 1));

  ok1(!(BrokenDate(2010, 1, 1) > BrokenDate(2010, 1, 1)));
  ok1(!(BrokenDate(2010, 1, 1) > BrokenDate(2010, 1, 2)));
  ok1(BrokenDate(2010, 2, 1) > BrokenDate(2010, 1, 1));

  ok1(!(BrokenDate(2010, 1, 1) > BrokenDate(2010, 1, 1)));
  ok1(!(BrokenDate(2010, 1, 1) > BrokenDate(2011, 1, 1)));
  ok1(BrokenDate(2011, 1, 1) > BrokenDate(2010, 1, 1));

  ok1(BrokenDate(2011, 1, 1).IsPlausible());
  ok1(BrokenDate(2011, 12, 31).IsPlausible());
  ok1(BrokenDate(2500, 1, 1).IsPlausible());
  ok1(BrokenDate(1800, 1, 1).IsPlausible());
  ok1(!BrokenDate(2501, 1, 1).IsPlausible());
  ok1(!BrokenDate(1799, 1, 1).IsPlausible());
  ok1(!BrokenDate(2011, 1, 0).IsPlausible());
  ok1(!BrokenDate(2011, 0, 1).IsPlausible());
  ok1(!BrokenDate(2011, 13, 1).IsPlausible());
  ok1(!BrokenDate(2011, 1, 32).IsPlausible());

  BrokenDate d(2010, 2, 27);
  d.IncrementDay();
  ok1(d == BrokenDate(2010, 2, 28));
  d.IncrementDay();
  ok1(d == BrokenDate(2010, 3, 1));
  d.DecrementDay();
  ok1(d == BrokenDate(2010, 2, 28));
  d.DecrementDay();
  ok1(d == BrokenDate(2010, 2, 27));

  d = BrokenDate(2010, 12, 31);
  d.IncrementDay();
  ok1(d == BrokenDate(2011, 1, 1));

  d = BrokenDate(2012, 2, 27);
  d.IncrementDay();
  ok1(d == BrokenDate(2012, 2, 28));
  d.IncrementDay();
  ok1(d == BrokenDate(2012, 2, 29));
  d.IncrementDay();
  ok1(d == BrokenDate(2012, 3, 1));

  ok1(BrokenDate(2012, 10, 31).DaysSince(BrokenDate(2012, 10, 31)) == 0);
  ok1(BrokenDate(2012, 10, 31).DaysSince(BrokenDate(2012, 10, 30)) == 1);
  ok1(BrokenDate(2012, 10, 31).DaysSince(BrokenDate(2012, 10, 1)) == 30);
  ok1(BrokenDate(2012, 10, 1).DaysSince(BrokenDate(2012, 10, 31)) == -30);
  ok1(BrokenDate(2012, 1, 1).DaysSince(BrokenDate(2011, 12, 31)) == 1);
  ok1(BrokenDate(2011, 12, 31).DaysSince(BrokenDate(2012, 1, 1)) == -1);
  ok1(BrokenDate(2013, 3, 1).DaysSince(BrokenDate(2013, 2, 28)) == 1);
  ok1(BrokenDate(2014, 1, 1).DaysSince(BrokenDate(2013, 1, 1)) == 365);
  ok1(BrokenDate(2012, 3, 1).DaysSince(BrokenDate(2012, 2, 28)) == 2);
  ok1(BrokenDate(2013, 1, 1).DaysSince(BrokenDate(2012, 1, 1)) == 366);
}

static void
TestTime()
{
  ok1(BrokenTime(12, 15).hour == 12);
  ok1(BrokenTime(12, 15).minute == 15);
  ok1(BrokenTime(12, 15).second == 0);

  ok1(BrokenTime(12, 15, 30).hour == 12);
  ok1(BrokenTime(12, 15, 30).minute == 15);
  ok1(BrokenTime(12, 15, 30).second == 30);

  ok1(BrokenTime(12, 15, 30) == BrokenTime(12, 15, 30));
  ok1(!(BrokenTime(12, 15, 30) == BrokenTime(12, 15, 31)));
  ok1(!(BrokenTime(12, 15, 30) == BrokenTime(12, 16, 30)));
  ok1(!(BrokenTime(12, 15, 30) == BrokenTime(13, 15, 30)));

  ok1(!(BrokenTime(12, 15, 30) > BrokenTime(12, 15, 30)));
  ok1(!(BrokenTime(12, 15, 30) > BrokenTime(12, 15, 31)));
  ok1(BrokenTime(12, 15, 31) > BrokenTime(12, 15, 30));

  ok1(!(BrokenTime(12, 15, 30) > BrokenTime(12, 15, 30)));
  ok1(!(BrokenTime(12, 15, 30) > BrokenTime(2010, 16, 30)));
  ok1(BrokenTime(12, 16, 30) > BrokenTime(12, 15, 30));

  ok1(!(BrokenTime(12, 15, 30) > BrokenTime(12, 15, 30)));
  ok1(!(BrokenTime(12, 15, 30) > BrokenTime(13, 15, 30)));
  ok1(BrokenTime(13, 15, 30) > BrokenTime(12, 15, 30));

  ok1(BrokenTime(23, 59, 59).IsPlausible());
  ok1(BrokenTime(0, 0, 0).IsPlausible());
  ok1(!BrokenTime(24, 0, 0).IsPlausible());
  ok1(!BrokenTime(12, 60, 1).IsPlausible());
  ok1(!BrokenTime(12, 15, 60).IsPlausible());

  ok1(BrokenTime(12, 15, 30).GetSecondOfDay() == 44130);
  ok1(BrokenTime::FromSecondOfDay(44130) == BrokenTime(12, 15, 30));
  ok1(BrokenTime::FromSecondOfDayChecked(130530) == BrokenTime(12, 15, 30));

  ok1(BrokenTime(12, 15, 30).GetMinuteOfDay() == 735);
  ok1(BrokenTime::FromMinuteOfDay(735) == BrokenTime(12, 15));
  ok1(BrokenTime::FromMinuteOfDayChecked(735) == BrokenTime(12, 15));

  ok1(BrokenTime(12, 15) + 120 == BrokenTime(12, 17));
  ok1(BrokenTime(23, 59) + 120 == BrokenTime(0, 1));
  ok1(BrokenTime(23, 59) + 120 == BrokenTime(0, 1));
  ok1(BrokenTime(0, 1) - 120 == BrokenTime(23, 59));
  ok1(BrokenTime(0, 1) - 120u == BrokenTime(23, 59));
}

static void
TestDateTime()
{
  ok1(BrokenDateTime(2010, 1, 2).year == 2010);
  ok1(BrokenDateTime(2010, 1, 2).month == 1);
  ok1(BrokenDateTime(2010, 1, 2).day == 2);
  ok1(BrokenDateTime(2010, 1, 2).hour == 0);
  ok1(BrokenDateTime(2010, 1, 2).minute == 0);
  ok1(BrokenDateTime(2010, 1, 2).second == 0);

  ok1(BrokenDateTime(2010, 1, 2, 12, 15).year == 2010);
  ok1(BrokenDateTime(2010, 1, 2, 12, 15).month == 1);
  ok1(BrokenDateTime(2010, 1, 2, 12, 15).day == 2);
  ok1(BrokenDateTime(2010, 1, 2, 12, 15).hour == 12);
  ok1(BrokenDateTime(2010, 1, 2, 12, 15).minute == 15);
  ok1(BrokenDateTime(2010, 1, 2, 12, 15).second == 0);

  ok1(BrokenDateTime(2010, 1, 2, 12, 15, 30).year == 2010);
  ok1(BrokenDateTime(2010, 1, 2, 12, 15, 30).month == 1);
  ok1(BrokenDateTime(2010, 1, 2, 12, 15, 30).day == 2);
  ok1(BrokenDateTime(2010, 1, 2, 12, 15, 30).hour == 12);
  ok1(BrokenDateTime(2010, 1, 2, 12, 15, 30).minute == 15);
  ok1(BrokenDateTime(2010, 1, 2, 12, 15, 30).second == 30);

  ok1(BrokenDateTime(2010, 2, 28, 23, 0, 0) == BrokenDateTime(2010, 2, 28, 23, 0, 0));
  ok1(BrokenDateTime(2010, 2, 28, 23, 0, 0) + 3600 == BrokenDateTime(2010, 3, 1));
  ok1(BrokenDateTime(2010, 2, 28, 23, 59, 59) + 1 == BrokenDateTime(2010, 3, 1));
  ok1(BrokenDateTime(2010, 2, 28, 23, 59, 59) + 2 == BrokenDateTime(2010, 3, 1, 0, 0, 1));
  ok1(BrokenDateTime(2010, 12, 31, 23, 59, 59) + 1 == BrokenDateTime(2011, 1, 1));

  ok1(BrokenDateTime(2010, 1, 2, 12, 15, 30).ToUnixTimeUTC() == 1262434530);

  ok1(BrokenDateTime(2010, 1, 1, 0, 0 ,1) -
      BrokenDateTime(2010, 1, 1, 0, 0 ,0) == 1);
  ok1(BrokenDateTime(2010, 1, 1, 0, 1 ,0) -
      BrokenDateTime(2010, 1, 1, 0, 0 ,0) == 60);
  ok1(BrokenDateTime(2010, 1, 1, 1, 0 ,0) -
      BrokenDateTime(2010, 1, 1, 0, 0 ,0) == 60 * 60);
  ok1(BrokenDateTime(2010, 1, 2, 0, 0 ,0) -
      BrokenDateTime(2010, 1, 1, 0, 0 ,0) == 60 * 60 * 24);
}

int main(int argc, char **argv)
{
  plan_tests(107);

  TestDate();
  TestTime();
  TestDateTime();

  return exit_status();
}
