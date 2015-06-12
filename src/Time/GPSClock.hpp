/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
  /**
   * A large negative value which ensure that first CheckAdvance()
   * call after Reset() returns true, even if starting XCSoar right
   * after midnight.
   */
  static constexpr int RESET_VALUE = -99999;

  fixed last;

public:
  /**
   * Initializes the object, setting the last time stamp to "0",
   * i.e. a check() will always succeed.  If you do not want this
   * default behaviour, call update() immediately after creating the
   * object.
   */
  GPSClock():last(RESET_VALUE) {}

  /**
   * Resets the clock.
   */
  void Reset() {
    last = fixed(RESET_VALUE);
  }

  /**
   * Updates the clock.
   */
  void Update(fixed now) {
    last = now;
  }

  /**
   * Checks whether the GPS time was reversed
   * @param now Current time
   * @return True if time has been reversed, False otherwise
   */
  bool CheckReverse(const fixed now) {
    if (now<last) {
      Update(now);
      return true;
    } else {
      return false;
    }
  }

  /**
   * Checks whether the specified duration (dt) has passed since the last
   * update. If yes, it updates the time stamp.
   * @param now Current time
   * @param dt The timestep in seconds
   * @return
   */
  bool CheckAdvance(const fixed now, const fixed dt) {
    if (CheckReverse(now))
      return false;

    if (now >= last + dt) {
      Update(now);
      return true;
    } else
      return false;
  }

  bool CheckAdvance(fixed now, unsigned dt) {
    return CheckAdvance(now, fixed(dt));
  }
};

#endif
