// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WrapClock.hpp"
#include "NMEA/Info.hpp"

using namespace std::chrono;

TimeStamp
WrapClock::Normalise(TimeStamp stamp, BrokenDate &date,
                     const BrokenTime &time) noexcept
{
  constexpr TimeStamp ONE_DAY{hours{24}};
  constexpr TimeStamp ONE_HOUR{hours{1}};

  assert(stamp.IsDefined());
  assert(time.IsPlausible());

  int days = 0;
  if (date.IsPlausible()) {
    if (last_input_date.IsPlausible() &&
        date.DaysSince(last_input_date) < 0)
      /* input date warp */
      Reset();

    if (last_output_date.IsPlausible()) {
      days = date.DaysSince(last_output_date);
      if (days > 0) {
        /* new date from GPS: copy it and update the day serial */
        last_day += days;
        last_output_date = date;

        if (days == 1 && last_stamp >= ONE_DAY - minutes{1} &&
            stamp >= last_stamp)
          stamp = {};
      } else if (days < 0 && !last_input_date.IsPlausible())
        /* time warp after recovering from invalid input date */
        Reset();
    }
  }

  last_input_date = date;

  if (stamp < last_stamp && days <= 0) {
    assert(last_stamp.IsDefined());

    if (stamp < ONE_HOUR && last_stamp >= ONE_DAY - hours{1}) {
      /* wraparound, but no date changed: assume the date was not yet
         updated, and wrap to the next day */
      ++last_day;

      if (date.IsPlausible())
        date.IncrementDay();

      if (last_output_date.IsPlausible())
        last_output_date.IncrementDay();
    } else if (stamp + seconds{2} >= last_stamp) {
      /* Ignore time warps of less than 2 seconds.

         This is used to reduce quirks when the time stamps in GPGGA
         and GPRMC are off by a second.  Without this workaround,
         XCSoar loses the GPS fix every now and then, because GPRMC is
         ignored most of the time */

      stamp = last_stamp;
    } else if (stamp + hours{12} < last_stamp) {
      /* big time warp */
      Reset();
    }
  } else if (days == -1 && last_output_date.IsPlausible())
    /* midnight wraparound was detected recently, but the GPS still
       hasn't sent a new date: return the date that was already
       incremented */
    date = last_output_date;

  last_stamp = stamp;

  if (!last_output_date.IsPlausible())
    /* initialise last_output_date on the first iteration or after a
       time warp */
    last_output_date = date;

  last_time = time;

  return stamp + last_day * hours{24};
}

void
WrapClock::Normalise(NMEAInfo &basic)
{
  if (basic.time_available)
    /* replace "time" with the normalised time stamp */
    basic.time = Normalise(basic.time, basic.date_time_utc,
                           basic.date_time_utc);
}

