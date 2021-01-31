/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_START_STATS_HPP
#define XCSOAR_START_STATS_HPP

#include <type_traits>

struct AircraftState;

/**
 * Container for start point statistics.
 */
struct StartStats {
  bool task_started;

  /**
   * The time when the task was started [UTC seconds of day].  Only
   * valid if #task_started is true.
   */
  double time;

  /**
   * The aircraft's altitude when the task was started [m MSL].  Only
   * valid if #task_started is true.
   */
  double altitude;

  /**
   * The aircraft's ground speed when the task was started [m/s].
   * Only valid if #task_started is true.
   */
  double ground_speed;

  void Reset() {
    task_started = false;
  }

  /**
   * Enable the #task_started flag and copy data from the
   * #AircraftState.
   */
  void SetStarted(const AircraftState &aircraft);
};

static_assert(std::is_trivial<StartStats>::value, "type is not trivial");

#endif
