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
#ifndef ORDEREDTASK_BEHAVIOUR_HPP
#define ORDEREDTASK_BEHAVIOUR_HPP

#include "Math/fixed.hpp"

struct AIRCRAFT_STATE;
struct TaskStartMargins;

/**
 * Settings for ordered tasks; most of these are set by
 * the #AbstractTaskFactory but can be overriden
 */
class OrderedTaskBehaviour
{
public:
  friend class Serialiser;
  OrderedTaskBehaviour();

  /**
   * Option to enable calculation of scores, and protect against
   * task changes if flight/task has started
   */
  bool task_scored;
  /** Desired AAT minimum task time (s) */
  fixed aat_min_time;
  /** Maximum ground speed (m/s) allowed in start sector */
  fixed start_max_speed;
  /** Maximum height (m) allowed in start sector */
  unsigned start_max_height;
  /** Reference for max start height (0=AGL, 1=abs) */
  unsigned start_max_height_ref;
  /** Minimum height AGL (m) allowed to finish */
  unsigned finish_min_height;
  /** Whether ordered task start and finish requires FAI height rules
   *  and (no) speed rule */
  bool fai_finish;
  /** Minimum number of turnpoints including start/finish */
  unsigned min_points;
  /** Maximum number of turnpoints including start/finish */
  unsigned max_points;
  /** Whether all turnpoints except start/finish are same type */
  bool homogeneous_tps;
  /** Whether start/finish turnpoints must be the same */
  bool is_closed;
  /** Whether start points needs to be armed */
  bool start_requires_arm;

  /**
   * Check whether aircraft speed is within start speed limits
   *
   * @param state Aircraft state
   * @param behaviour TaskBehaviour (contains margins)
   * @param with_margin Whether to use margin for minor rule violation
   *
   * @return True if within limits
   */
  bool check_start_speed(const AIRCRAFT_STATE &state, 
                         const TaskStartMargins &margins,
                         const bool with_margin=false) const;

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
  bool check_start_height(const AIRCRAFT_STATE &state,
                          const TaskStartMargins &margins,
                          const fixed spAlt,
                          const bool with_margin=false) const;

  /**
   * Check whether aircraft height is within finish height limit
   *
   * @param state Aircraft state
   * @param fpAlt finish point altitude
   *
   * @return True if within limits
   */
  bool check_finish_height(const AIRCRAFT_STATE &state, const fixed fpAlt) const;


  /**
   * Convenience function (used primarily for testing) to disable
   * all expensive task behaviour functions.
   */
  void all_off() {
    task_scored = false;
  }

  /**
   * Determine if the task should have a fixed number of turnpoints
   *
   * @return True if task is fixed size
   */
  bool is_fixed_size() const {
    return min_points == max_points;
  }
};

#endif
