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
#ifndef TASKSTATS_HPP
#define TASKSTATS_HPP

#include "ElementStat.hpp"
#include "StartStats.hpp"
#include "WindowStats.hpp"

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
  double glide_required;
  /** Calculated cruise efficiency ratio */
  double cruise_efficiency;
  /** Calculated effective MC (m/s) */
  double effective_mc;
  /** Best MacCready setting calculated for final glide (m/s) */
  double mc_best;

  /** Nominal task distance (m) */
  double distance_nominal;
  /** Maximum achievable task distance (m) */
  double distance_max;
  /** Minimum achievable task distance (m) */
  double distance_min;
  /** Scored distance (m) */
  double distance_scored;

  /**
   * Calculated instantaneous speed (m/s).  Negative if unknown.
   */
  double inst_speed_slow;
  double inst_speed_fast;

  /**
   * Index of the active task point.
   */
  unsigned active_index;

  /** Whether the task is navigable */
  bool task_valid;

  /** Whether ordered task has AAT areas */
  bool has_targets;

  /**
   * Is this a MAT task?
   */
  bool is_mat;

  /** Whether ordered task has optional starts */
  bool has_optional_starts;

  /** Whether the task is finished */
  bool task_finished;

  /**
   * Is the aircraft currently inside the current task point's
   * observation zone?
   */
  bool inside_oz;

  /**
   * Does the current task point need to be armed?
   */
  bool need_to_arm;

  /** Whether the task is appoximately in final glide */
  bool flight_mode_final_glide;

  StartStats start;

  WindowStats last_hour;

  double GetEstimatedTotalTime() const {
    return total.time_elapsed + total.time_remaining_start;
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
