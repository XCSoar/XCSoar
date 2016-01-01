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

#ifndef CONTEST_RESULT_HPP
#define CONTEST_RESULT_HPP

#include <type_traits>

struct ContestResult
{
  /** Score (pts) according to OLC rule */
  double score;
  /** Optimum distance (m) travelled according to OLC rule */
  double distance;
  /** Time (s) of optimised OLC path */
  double time;

  void Reset() {
    score = 0;
    distance = 0;
    time = 0;
  }

  constexpr bool IsDefined() const {
    return score > 0;
  }

  /**
   * Returns the average speed on the optimised path [m/s].  Returns
   * zero if the result is invalid.
   */
  constexpr double GetSpeed() const {
    return time > 0
      ? distance / time
      : 0.;
  }
};

static_assert(std::is_trivial<ContestResult>::value, "type is not trivial");

#endif
