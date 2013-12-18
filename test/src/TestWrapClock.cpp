/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#define ACCURACY 1000000

#include "Time/BrokenDate.hpp"
#include "Time/BrokenTime.hpp"
#include "Time/WrapClock.hpp"
#include "TestUtil.hpp"

static fixed
Normalise(WrapClock &w, BrokenDate &date,
          unsigned hour, unsigned minute, unsigned second=0)
{
  return w.Normalise(fixed(hour * 3600 + minute * 60 + second), date,
                     BrokenTime(hour, minute, second));
}

static void
TestBasic()
{
  WrapClock w;
  w.Reset();

  BrokenDate date = BrokenDate::Invalid();

  ok1(equals(Normalise(w, date, 0, 1), 60));
  ok1(!date.IsPlausible());

  ok1(equals(Normalise(w, date, 12, 0), 12 * 3600));
  ok1(!date.IsPlausible());

  /* time warp */
  ok1(equals(Normalise(w, date, 1, 0), 3600));
  ok1(!date.IsPlausible());

  /* seek right before midnight and produce a midnight wraparound */
  ok1(equals(Normalise(w, date, 23, 50), 23 * 3600 + 50 * 60));
  ok1(!date.IsPlausible());

  ok1(equals(Normalise(w, date, 0, 10), 24 * 3600 + 10 * 60));
  ok1(!date.IsPlausible());

  /* .. and another one */
  ok1(equals(Normalise(w, date, 23, 50), 47 * 3600 + 50 * 60));
  ok1(!date.IsPlausible());

  ok1(equals(Normalise(w, date, 0, 10), 48 * 3600 + 10 * 60));
  ok1(!date.IsPlausible());

  /* inject a valid date */
  date = BrokenDate(2013, 5, 14);
  BrokenDate expected_date = date;
  ok1(equals(Normalise(w, date, 0, 10), 48 * 3600 + 10 * 60));
  ok1(date == expected_date);

  /* same day */
  date = BrokenDate(2013, 5, 14);
  ok1(equals(Normalise(w, date, 0, 20), 48 * 3600 + 20 * 60));
  ok1(date == expected_date);

  /* next day */
  expected_date = date = BrokenDate(2013, 5, 15);
  ok1(equals(Normalise(w, date, 0, 10), 72 * 3600 + 10 * 60));
  ok1(date == expected_date);

  /* fast-forward 3 days */
  expected_date = date = BrokenDate(2013, 5, 18);
  ok1(equals(Normalise(w, date, 0, 10), 144 * 3600 + 10 * 60));
  ok1(date == expected_date);

  /* back one day */
  expected_date = date = BrokenDate(2013, 5, 17);
  ok1(equals(Normalise(w, date, 0, 10), 10 * 60));
  ok1(date == expected_date);

  /* fast-forward 2 days */
  expected_date = date = BrokenDate(2013, 5, 19);
  ok1(equals(Normalise(w, date, 0, 10), 48 * 3600 + 10 * 60));
  ok1(date == expected_date);

  /* back to invalid date */
  date = BrokenDate::Invalid();
  ok1(equals(Normalise(w, date, 0, 20), 48 * 3600 + 20 * 60));
  ok1(!date.IsPlausible());

  /* produce a wraparound and then recover date */
  ok1(equals(Normalise(w, date, 23, 50), 71 * 3600 + 50 * 60));
  ok1(equals(Normalise(w, date, 0, 10), 72 * 3600 + 10 * 60));
  ok1(equals(Normalise(w, date, 0, 20), 72 * 3600 + 20 * 60));
  ok1(!date.IsPlausible());

  expected_date = date = BrokenDate(2013, 5, 20);
  ok1(equals(Normalise(w, date, 0, 20), 72 * 3600 + 20 * 60));
  ok1(date == expected_date);

  /* back to invalid date, wraparound and then recover with warped
     date (resets the WrapClock) */
  date = BrokenDate::Invalid();
  ok1(equals(Normalise(w, date, 0, 20), 72 * 3600 + 20 * 60));
  ok1(equals(Normalise(w, date, 23, 50), 95 * 3600 + 50 * 60));
  ok1(equals(Normalise(w, date, 0, 10), 96 * 3600 + 10 * 60));
  ok1(!date.IsPlausible());

  expected_date = date = BrokenDate(2013, 5, 20);
  ok1(equals(Normalise(w, date, 0, 20), 20 * 60));
  ok1(date == expected_date);

  /* wrap midnight, but don't increment date - WrapClock must
     increment the date automatically */

  w.Reset();
  expected_date = date = BrokenDate(2013, 2, 28);
  ok1(equals(Normalise(w, date, 23, 55), 23 * 3600 + 55 * 60));
  ok1(date == expected_date);

  expected_date = BrokenDate(2013, 3, 1);
  ok1(equals(Normalise(w, date, 0, 5), 24 * 3600 + 5 * 60));
  ok1(date == expected_date);
}

int main(int argc, char **argv)
{
  plan_tests(44);

  TestBasic();

  return exit_status();
}
