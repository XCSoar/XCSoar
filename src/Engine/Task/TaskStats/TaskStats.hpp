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
#ifndef TASKSTATS_HPP
#define TASKSTATS_HPP

#include "ElementStat.hpp"

#ifdef DO_PRINT
#include <iostream>
#endif

/**
 * Container for common task statistics
 */
class TaskStats 
{
public:
  ElementStat total; /**< Total task statistics */
  ElementStat current_leg; /**< Current (active) leg statistics */

  fixed Time; /**< Global time (UTC, s) of last update */

  // calculated values
  fixed glide_required; /**< Calculated glide angle required */
  fixed cruise_efficiency; /**< Calculated cruise efficiency ratio */
  fixed effective_mc; /**< Calculated effective MC (m/s) */
  fixed mc_best; /**< Best MacCready setting calculated for final glide (m/s) */

  fixed distance_nominal; /**< Nominal task distance (m) */
  fixed distance_max; /**< Maximum achievable task distance (m) */
  fixed distance_min; /**< Minimum achievable task distance (m) */
  fixed distance_scored; /**< Scored distance (m) */

  bool task_valid; /**< Whether the task is navigable */
  bool task_started; /**< Whether the task is started */
  bool task_finished; /**< Whether the task is finished */
  bool has_targets; /**< Whether the task has adjustable targets */
  bool flight_mode_final_glide; /**< Whether the task is appoximately in final glide */
  int flight_mode_height_margin; /**< Margin for final glide flight mode transition (m) */

/** 
 * Constructor.  Initialises all to zero.
 * 
 */
  TaskStats();

  fixed get_pirker_speed() const; /**< Incremental task speed adjusted for mc,
                                   * target changes */

/** 
 * Reset each element (for incremental speeds).
 * 
 */
  void reset();

/** 
 * Convenience function, determines if remaining task is in final glide
 * 
 * @return True if is mode changed
 */
  bool calc_flight_mode();

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const TaskStats& ts);
#endif
};

class TaskStatsComputer {
  TaskStats &data;
public:
  ElementStatComputer total;
  ElementStatComputer current_leg;

public:
  TaskStatsComputer(TaskStats &_data)
    :data(_data),
     total(data.total), current_leg(data.current_leg) {}

  /**
   * Reset each element (for incremental speeds).
   *
   */
  void reset();
};

#endif
