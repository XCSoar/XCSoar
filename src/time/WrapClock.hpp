// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BrokenDate.hpp"
#include "BrokenTime.hpp"
#include "Stamp.hpp"

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
  TimeStamp last_stamp;

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
    last_stamp = TimeStamp::Undefined();
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
  TimeStamp Normalise(TimeStamp stamp, BrokenDate &date,
                      const BrokenTime &time) noexcept;

  /**
   * Convenience wrapper that takes a #NMEAInfo and updates its "time"
   * attribute.
   */
  void Normalise(NMEAInfo &basic);
};
