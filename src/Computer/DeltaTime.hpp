/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_DELTA_TIME_HPP
#define XCSOAR_DELTA_TIME_HPP

#include "Math/fixed.hpp"

/**
 * Calculates the time difference between two events, and detects time
 * warps.
 */
class DeltaTime {
  /**
   * The time stamp of the previous call.  A negative value means
   * "unavailable".
   */
  fixed last_time;

public:
  void Reset() {
    last_time = fixed(-1);
  }

  gcc_pure
  bool IsDefined() const {
    return !negative(last_time);
  }

  /**
   * Update the "last" time stamp, and return the difference.  Returns
   * -1 on time warp.
   *
   * @param current_time the current time stamp or -1 if not known
   * @param min_delta returns zero and does not update the "last" time
   * stamp if the difference is smaller than this value
   * @param warp_tolerance if the time warp is smaller than this
   * value, then zero is returned instead of -1
   * @return the (non-negative) time stamp difference since the last
   * call, or 0 if difference is too small, or -1 on time warp
   */
  fixed Update(fixed current_time, fixed min_delta, fixed warp_tolerance) {
    assert(!negative(current_time));
    assert(!negative(min_delta));
    assert(!negative(warp_tolerance));

    if (!IsDefined()) {
      /* first call */
      last_time = current_time;
      return fixed(0);
    }

    if (current_time < last_time) {
      /* time warp */

      const fixed delta = last_time - current_time;
      last_time = current_time;
      return delta < warp_tolerance ? fixed(0) : fixed(-1);
    }

    const fixed delta = current_time - last_time;
    if (delta < min_delta)
      /* difference too small, don't update "last" time stamp to let
         small differences add up eventually */
      return fixed(0);

    last_time = current_time;

    if (delta > fixed(4 * 3600))
      /* after several hours without a signal, we can assume there was
         a time warp */
      return fixed(-1);

    return delta;
  }
};

#endif
