/* Copyright_License {

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

#ifndef XCSOAR_START_CONSTRAINTS_HPP
#define XCSOAR_START_CONSTRAINTS_HPP

#include "Math/fixed.hpp"
#include "Time/RoughTime.hpp"
#include "Geo/AltitudeReference.hpp"

struct AircraftState;
struct TaskStartMargins;

struct StartConstraints {
  /**
   * The time span during which the start gate is open.
   */
  RoughTimeSpan open_time_span;

  /** Maximum ground speed (m/s) allowed in start sector */
  fixed max_speed;

  /** Maximum height (m) allowed in start sector */
  unsigned max_height;

  /** Reference for max start height */
  AltitudeReference max_height_ref;

  /**
   * This is a copy of FinishConstraints::fai_finish.  If true, then
   * the constraints defined in this class will be ignored.
   */
  bool fai_finish;

  void SetDefaults();

  /**
   * Check whether aircraft speed is within start speed limits
   *
   * @param state Aircraft state
   * @param behaviour TaskBehaviour (contains margins)
   * @param with_margin Whether to use margin for minor rule violation
   *
   * @return True if within limits
   */
  gcc_pure
  bool CheckSpeed(const AircraftState &state,
                  const TaskStartMargins &margins,
                  const bool with_margin = false) const;

  /**
   * Check whether aircraft height is within start height limit
   *
   * @param state Aircraft state
   * @param behaviour TaskBehaviour (contains margins)
   * @param spAlt start point altitude
   * @param with_margin Whether to use margin for minor rule violation
   *
   * @return True if within limits
   */
  gcc_pure
  bool CheckHeight(const AircraftState &state,
                   const TaskStartMargins &margins,
                   const fixed start_elevation,
                   const bool with_margin = false) const;
};

#endif
