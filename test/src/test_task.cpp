// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_task.hpp"
#include "harness_waypoints.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "test_debug.hpp"

extern "C" {
#include "tap.h"
}

int main(int argc, char** argv)
{
  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  static constexpr unsigned NUM_RANDOM = 50;
  static constexpr unsigned NUM_TYPE_MANIPS = 50;
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

  for (unsigned i=0; i<NUM_TASKS+2; i++) {
    TaskManager task_manager(task_behaviour, waypoints);
    task_manager.SetGlidePolar(glide_polar);
    ok(test_task(task_manager, waypoints, i),GetTestName("construction",i,0),0);
  }

  for (unsigned i=0; i<NUM_RANDOM; i++) {
    TaskManager task_manager(task_behaviour, waypoints);
    task_manager.SetGlidePolar(glide_polar);
    ok(test_task(task_manager, waypoints, 7),GetTestName("construction",7,0),0);
  }

  return exit_status();
}

