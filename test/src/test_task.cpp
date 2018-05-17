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

#include "harness_task.hpp"
#include "harness_waypoints.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "test_debug.hpp"

int main(int argc, char** argv)
{
  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  #define NUM_RANDOM 50
  #define NUM_TYPE_MANIPS 50
  plan_tests(NUM_TASKS+2+NUM_RANDOM+8+NUM_TYPE_MANIPS);

  GlidePolar glide_polar(2);

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();

  Waypoints waypoints;
  SetupWaypoints(waypoints);

  {
    TaskManager task_manager(task_behaviour, waypoints);
    task_manager.SetGlidePolar(glide_polar);
    test_task_bad(task_manager,waypoints);
  }

  {
    for (unsigned i = 0; i < NUM_TYPE_MANIPS; i++) {
      TaskManager task_manager(task_behaviour, waypoints);
      ok(test_task_type_manip(task_manager,waypoints, i+2), "construction: task type manip", 0);
    }
  }

  for (int i=0; i<NUM_TASKS+2; i++) {
    TaskManager task_manager(task_behaviour, waypoints);
    task_manager.SetGlidePolar(glide_polar);
    ok(test_task(task_manager, waypoints, i),GetTestName("construction",i,0),0);
  }

  for (int i=0; i<NUM_RANDOM; i++) {
    TaskManager task_manager(task_behaviour, waypoints);
    task_manager.SetGlidePolar(glide_polar);
    ok(test_task(task_manager, waypoints, 7),GetTestName("construction",7,0),0);
  }

  return exit_status();
}

