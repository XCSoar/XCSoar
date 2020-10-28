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

#ifndef XCSOAR_WRAP_CLOCK_HPP
#define XCSOAR_WRAP_CLOCK_HPP

#include "BrokenDate.hpp"
#include "BrokenTime.hpp"

struct NMEAInfo;

/**
 * This class computes a consistently ascending time stamp from a
 * second-of-day stamp and a #BrokenDateTime.  It handles midnight
 * wraparound by adding 86400 to the time stamp for each day that
 * passed since the object was last reset.
 *
 * There is no implicit initialisation.  To initialise a new object,
 * call Reset().
 */
class WrapClock {
  /**
   * A serial number for the day specified by #last_output_date (at
   * midnight), beginning at 0 for the first day of flight.
   */
  unsigned last_day;

  /**
   * The last time_of_day parameter.  This is negative if Normalise()
   * was never called since the most recent Reset() call.
   */
  double last_stamp;

  /**
   * The last known input date.  Check IsPlausible() before using this
   * attribute.
   */
  BrokenDate last_input_date;

  /**
   * The last known output date.  Check IsPlausible() before using
   * this attribute.
   */
  BrokenDate last_output_date;

  /**
   * The last known time of day.  Check IsPlausible() before using
   * this attribute.
   */
  BrokenTime last_time;

public:
  void Reset() {
    last_day = 0;
    last_stamp = -1;
    last_input_date = last_output_date = BrokenDate::Invalid();
    last_time = BrokenTime::Invalid();
  }

  /**
   * Translate the given time stamp to make it ascending and linear
   * even during a midnight wraparound.
   *
   * @param stamp the time of day [seconds]
   * @param date the current date; may be BrokenDate::Invalid(); this
   * method may edit the date if the time has wrapped around, but the
   * date hasn't changed since the last call (if a new time was
   * received from the GPS, but the new date was not yet)
   * @param time the broken time of day; must be valid
   * @return a normalised time stamp [seconds] that is ascending and
   * linear (unless a "time warp" is observed)
   */
  double Normalise(double stamp, BrokenDate &date, const BrokenTime &time);

  /**
   * Convenience wrapper that takes a #NMEAInfo and updates its "time"
   * attribute.
   */
  void Normalise(NMEAInfo &basic);
};

#endif
