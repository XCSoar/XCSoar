/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef XCSOAR_GPS_CLOCK_HPP
#define XCSOAR_GPS_CLOCK_HPP

#include "Math/fixed.hpp"

/**
 * Class for GPS-time based time intervals
 */
class GPSClock {
private:
  fixed last;
  fixed dt;

public:
  /**
   * Initializes the object, setting the last time stamp to "0",
   * i.e. a check() will always succeed.  If you do not want this
   * default behaviour, call update() immediately after creating the
   * object.
   */
  GPSClock(const fixed _minstep):last(fixed_zero), dt(_minstep) {}

  /**
   * Resets the clock.
   */
  void reset() {
    last = fixed_zero;
  }

  /**
   * Checks whether the GPS time was reversed
   * @param now Current time
   * @return True if time has been reversed, False otherwise
   */
  bool check_reverse(const fixed now) {
    if (now<last) {
      last=now;
      return true;
    } else {
      return false;
    }
  }

  /**
   * Set dt to a new value defined by _dt
   * @param _dt The new value fot dt
   */
  void set_dt(const fixed _dt) {
    dt = _dt;
  }

  /**
   * Calls check_advance(fixed, fixed) with dt
   * as the default value for dt
   * @param now Current time
   * @see check_advance(fixed, fixed)
   */
  bool check_advance(const fixed now) {
    return check_advance(now, dt);
  }

  fixed delta_advance(const fixed now) {
    fixed dt=now-last;
    if (check_reverse(now)) {
      return fixed_minus_one;
    }
    // check if time has advanced past dt
    if (now-last>=dt) {
      last= now;
      return dt;
    } else {
      return fixed_zero;
    }
  }

  /**
   * Checks whether the specified duration (dt) has passed since the last
   * update. If yes, it updates the time stamp.
   * @param now Current time
   * @param dt The timestep in milliseconds
   * @return
   */
  bool check_advance(const fixed now, const fixed dt) {
    if (check_reverse(now)) {
      return false;
    }
    if (now >= last + dt) {
      last = now;
      return true;
    } else
      return false;
  }
};

#endif
