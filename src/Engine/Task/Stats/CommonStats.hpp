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
#ifndef COMMON_STATS_HPP
#define COMMON_STATS_HPP

#include "Time/RoughTime.hpp"
#include "Geo/GeoVector.hpp"
#include "TaskSummary.hpp"
#include "../TaskType.hpp"

#include <type_traits>

/** 
 * Task statistics that are common across all managed tasks.
 * This is used for statistics for which it makes no sense to
 * have per-task instances, and where access to certain statistics
 * is required whatever mode the task manager is in.
 */
class CommonStats 
{
public:
  /**
   * A copy of #StartConstraints::open_time_span.
   */
  RoughTimeSpan start_open_time_span;

  /** Whether the task found landable reachable waypoints (aliases abort) */
  bool landable_reachable;
  /** time UTC ship descended through max task start height */
  double TimeUnderStartMaxHeight;
  /** Time (s) until assigned minimum time is achieved */
  double aat_time_remaining;
  /** Average speed over target task distance at minimum assigned time + margin (m/s) */
  double aat_speed_target;
  /** Average speed over max task at minimum assigned time + margin (m/s) */
  double aat_speed_max;
  /** Average speed over min task at minimum assigned time + margin (m/s) */
  double aat_speed_min;
  /** Vector to home waypoint */
  GeoVector vector_home;

  /** The current task type/mode */
  TaskType task_type;

  /** Is there a tp after this */
  bool active_has_next;
  /** Is there a tp before this */
  bool active_has_previous;
  /** Is next turnpoint the final */
  bool next_is_last;
  /** Is previous turnpoint the first */
  bool previous_is_first;

  /** Block speed to fly */
  double V_block;
  /** Dolphin speed to fly */
  double V_dolphin;

  /** Risk MC setting (m/s) */
  double current_risk_mc;

  /** Working height floor (m MSL) */
  double height_min_working;

  /** Working height ceiling (m MSL) */
  double height_max_working;

  /** Ratio of current height above working floor to working height band */
  double height_fraction_working;

  /** Summary of ordered task progress */
  TaskSummary ordered_summary;

  /**
   * Reset the stats as if never flown
   */
  void Reset();

  /**
   * Reset the task stats
   */
  void ResetTask();

  /**
   * Automatic positive vario scale from history [m/s]
   */
  double vario_scale_positive;

  /**
   * Automatic negative vario scale from history [m/s]
   */
  double vario_scale_negative;
};

static_assert(std::is_trivial<CommonStats>::value, "type is not trivial");

#endif
