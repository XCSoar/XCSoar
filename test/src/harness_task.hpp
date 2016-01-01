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

#ifndef TEST_TASK_HPP
#define TEST_TASK_HPP

#include "Task/TaskManager.hpp"

#define NUM_TASKS 5

bool test_task_mixed(TaskManager& task_manager,
                     const Waypoints &waypoints);

bool test_task_fai(TaskManager& task_manager,
                     const Waypoints &waypoints);

bool test_task_aat(TaskManager& task_manager,
                   const Waypoints &waypoints);

bool test_task_or(TaskManager& task_manager,
                  const Waypoints &waypoints);

bool test_task_dash(TaskManager& task_manager,
                  const Waypoints &waypoints);

bool test_task_manip(TaskManager& task_manager,
                     const Waypoints &waypoints);

/**
 * creates a random task of type (RT/AAT/FAI)
 * changes it to a random type of (RT/AAT/FAI)
 * mutates and validates the resulting tp types
 */
bool test_task_type_manip(TaskManager& task_manager,
                     const Waypoints &waypoints, unsigned n_points);

bool test_task_fg(TaskManager& task_manager,
                  const Waypoints &waypoints);

bool test_task_random(TaskManager& task_manager,
                      const Waypoints &waypoints,
                      const unsigned num_points);

/**
 * used for testing xcsoar6.1 that only has three types
 * of tasks RT, AAT, FAI
 * Generates random task type with valid random
 * start/finish/intermediate points, sizes etc
 */
bool test_task_random_RT_AAT_FAI(TaskManager& task_manager,
                      const Waypoints &waypoints,
                      const unsigned num_points);

bool test_task(TaskManager& task_manager,
               const Waypoints &waypoints,
               int test_num);

bool test_task_bad(TaskManager& task_manager,
                   const Waypoints &waypoints);

void test_note(const char* text);

void
task_report(const TaskManager &task_manager, const char *text);

const char* task_name(int test_num);

WaypointPtr random_waypoint(const Waypoints &waypoints);

#endif
