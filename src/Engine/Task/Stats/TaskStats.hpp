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
#ifndef TASKSTATS_HPP
#define TASKSTATS_HPP

#include "ElementStat.hpp"
#include "StartStats.hpp"
#include "WindowStats.hpp"
#include "Task/Computer/WindowStatsComputer.hpp"

#include <type_traits>

struct TaskBehaviour;

/** Container for common task statistics */
class TaskStats 
{
public:
  /** Total task statistics */
  ElementStat total;
  /** Current (active) leg statistics */
  ElementStat current_leg;

  /** Calculated glide angle required */
  fixed glide_required;
  /** Calculated cruise efficiency ratio */
  fixed cruise_efficiency;
  /** Calculated effective MC (m/s) */
  fixed effective_mc;
  /** Best MacCready setting calculated for final glide (m/s) */
  fixed mc_best;

  /** Nominal task distance (m) */
  fixed distance_nominal;
  /** Maximum achievable task distance (m) */
  fixed distance_max;
  /** Minimum achievable task distance (m) */
  fixed distance_min;
  /** Scored distance (m) */
  fixed distance_scored;

  /**
   * Index of the active task point.
   */
  unsigned active_index;

  /** Whether the task is navigable */
  bool task_valid;
  /** Whether the task is finished */
  bool task_finished;

  /**
   * Is the aircraft currently inside the current task point's
   * observation zone?
   */
  bool inside_oz;

  /** Whether the task is appoximately in final glide */
  bool flight_mode_final_glide;

  StartStats start;

  WindowStats last_hour;

  fixed GetEstimatedTotalTime() const {
    return total.time_elapsed + total.time_remaining_start;
  }

  /**
   * Check whether get_pirker_speed() is available.
   */
  bool IsPirkerSpeedAvailable() const {
    return total.pirker.IsDefined();
  }

  /** Incremental task speed adjusted for mc, target changes */
  fixed get_pirker_speed() const {
    return total.pirker.GetSpeedIncremental();
  }

  /** Reset each element (for incremental speeds). */
  void reset();

  /**
   * Convenience function, determines if remaining task is in final glide
   *
   * @return True if is mode changed
   */
  bool calc_flight_mode(const TaskBehaviour &settings);
};

static_assert(std::is_trivial<TaskStats>::value, "type is not trivial");

#endif
