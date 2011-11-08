/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#ifndef COMMON_STATS_HPP
#define COMMON_STATS_HPP

#include "Math/fixed.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "Util/TypeTraits.hpp"
#include "TaskSummary.hpp"

#ifdef DO_PRINT
#include <iostream>
#endif

/** 
 * Task statistics that are common across all managed tasks.
 * This is used for statistics for which it makes no sense to
 * have per-task instances, and where access to certain statistics
 * is required whatever mode the task manager is in.
 */
class CommonStats 
{
public:
  /** Whether the task found landable reachable waypoints (aliases abort) */
  bool landable_reachable;
  /** Whether the task is started (aliases ordered task) */
  bool task_started;
  /** time UTC ship descended through max task start height */
  fixed TimeUnderStartMaxHeight;
  /** Whether the task is finished (aliases ordered task) */
  bool task_finished;
  /** Time (s) until assigned minimum time is achieved */
  fixed aat_time_remaining;
  /**
   * Speed to achieve remaining task in minimum assigned time (m/s),
   * negative if already beyond minimum time
   */
  fixed aat_speed_remaining;
  /** Average speed over max task at minimum assigned time (m/s) */
  fixed aat_speed_max;
  /** Average speed over min task at minimum assigned time (m/s) */
  fixed aat_speed_min;
  /** Time (s) remaining for ordered task */
  fixed task_time_remaining;
  /** Time (s) elapsed for ordered task */
  fixed task_time_elapsed;
  /** Vector to home waypoint */
  GeoVector vector_home;
  /** Whether task is abort mode */
  bool mode_abort;
  /** Whether task is goto mode */
  bool mode_ordered;
  /** Whether task is goto mode */
  bool mode_goto;
  /** Whether ordered task is valid */
  bool ordered_valid;
  /** Whether ordered task has AAT areas */
  bool ordered_has_targets;
  /** Whether ordered task has optional starts */
  bool ordered_has_optional_starts;

  /** Is there a tp after this */
  bool active_has_next;
  /** Is there a tp before this */
  bool active_has_previous;
  /** Is next turnpoint the final */
  bool next_is_last;
  /** Is previous turnpoint the first */
  bool previous_is_first;
  /** index of active tp */
  unsigned active_taskpoint_index;

  /** Block speed to fly */
  fixed V_block;
  /** Dolphin speed to fly */
  fixed V_dolphin;

  /** MC setting at last update (m/s) */
  fixed current_mc;
  /** Risk MC setting (m/s) */
  fixed current_risk_mc;
  /** Bugs setting at last update */
  fixed current_bugs;
  /** Ballast setting at last update */
  fixed current_ballast;

  /** Glide solution flying terminally to active waypoint */
  GlideResult next_solution;

  /** Summary of ordered task progress */
  TaskSummary ordered_summary;

  /**
   * Reset the stats as if never flown
   */
  void reset();

  /**
   * Reset the task stats
   */
  void reset_task();

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, const CommonStats& ts);
#endif
};

static_assert(is_trivial<CommonStats>::value, "type is not trivial");

#endif
