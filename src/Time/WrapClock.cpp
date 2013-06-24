/*
Copyright_License {

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

#include "WrapClock.hpp"
#include "NMEA/Info.hpp"

fixed
WrapClock::Normalise(fixed stamp, BrokenDate &date, const BrokenTime &time)
{
  constexpr unsigned SECONDS_PER_HOUR = 60 * 60;
  constexpr unsigned SECONDS_PER_DAY = 24 * SECONDS_PER_HOUR;

  assert(!negative(stamp));
  assert(time.IsPlausible());

  int days = 0;
  if (last_date.IsPlausible() && date.IsPlausible()) {
    days = date.DaysSince(last_date);
    if (days < 0)
      /* time warp */
      Reset();
    else
      /* update the day serial for the new date */
      last_day += days;
  }

  bool increment_day = false;
  if (stamp < last_stamp && days <= 0) {
    assert(!negative(last_stamp));
    assert(days == 0);

    if (stamp < fixed(SECONDS_PER_HOUR) &&
        last_stamp >= fixed(SECONDS_PER_DAY - SECONDS_PER_HOUR)) {
      /* wraparound, but no date changed: assume the date was not yet
         updated, and wrap to the next day */
      increment_day = true;
      ++last_day;
    } else if (stamp + fixed(2) >= last_stamp) {
      /* Ignore time warps of less than 2 seconds.

         This is used to reduce quirks when the time stamps in GPGGA
         and GPRMC are off by a second.  Without this workaround,
         XCSoar loses the GPS fix every now and then, because GPRMC is
         ignored most of the time */

      stamp = last_stamp;
    } else if (stamp + fixed(12 * 3600) < last_stamp) {
      /* big time warp */
      Reset();
    }
  }

  last_stamp = stamp;

  if (date.IsPlausible()) {
    last_date = date;

    if (increment_day)
      date.IncrementDay();
  }

  if (increment_day && last_date.IsPlausible())
    last_date.IncrementDay();

  last_time = time;

  return stamp + fixed(last_day * SECONDS_PER_DAY);
}

void
WrapClock::Normalise(NMEAInfo &basic)
{
  if (basic.time_available)
    /* replace "time" with the normalised time stamp */
    basic.time = Normalise(basic.time, basic.date_time_utc,
                           basic.date_time_utc);
}

