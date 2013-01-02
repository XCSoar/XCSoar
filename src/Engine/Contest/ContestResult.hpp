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

#ifndef CONTEST_RESULT_HPP
#define CONTEST_RESULT_HPP

#include "Util/TypeTraits.hpp"
#include "Math/fixed.hpp"

struct ContestResult
{
  /** Score (pts) according to OLC rule */
  fixed score;
  /** Optimum distance (m) travelled according to OLC rule */
  fixed distance;
  /** Time (s) of optimised OLC path */
  fixed time;

  void Reset() {
    score = fixed_zero;
    distance = fixed_zero;
    time = fixed_zero;
  }

  bool IsDefined() const {
    return positive(score);
  }

  /**
   * Returns the average speed on the optimised path [m/s].  Returns
   * zero if the result is invalid.
   */
  gcc_pure
  fixed GetSpeed() const {
    return positive(time)
      ? distance / time
      : fixed_zero;
  }
};

static_assert(is_trivial<ContestResult>::value, "type is not trivial");

#endif
