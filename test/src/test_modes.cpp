/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Math/FastMath.h"
#include "TaskEventsPrint.hpp"
#include "harness_flight.hpp"

static bool
test_abort(int n_wind)
{
  GlidePolar glide_polar(fixed_two);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose)
    distance_counts();

  TaskEventsPrint default_events(verbose);

  TaskManager task_manager(default_events, waypoints);

  TaskBehaviour task_behaviour = task_manager.GetTaskBehaviour();
  task_behaviour.DisableAll();
  task_behaviour.enable_trace = false;
  task_manager.SetTaskBehaviour(task_behaviour);

  task_manager.SetGlidePolar(glide_polar);

  test_task(task_manager, waypoints, 1);

  task_manager.Abort();
  task_report(task_manager, "abort");

  autopilot_parms.goto_target = true;
  return run_flight(task_manager, autopilot_parms, n_wind);
}

static bool
test_goto(int n_wind, unsigned id, bool auto_mc)
{
  GlidePolar glide_polar(fixed_two);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose)
    distance_counts();

  TaskEventsPrint default_events(verbose);

  TaskManager task_manager(default_events, waypoints);

  TaskBehaviour task_behaviour = task_manager.GetTaskBehaviour();
  task_behaviour.DisableAll();
  task_behaviour.auto_mc = auto_mc;
  task_behaviour.enable_trace = false;
  task_manager.SetTaskBehaviour(task_behaviour);

  task_manager.SetGlidePolar(glide_polar);

  test_task(task_manager, waypoints, 1);

  task_manager.DoGoto(*waypoints.lookup_id(id));
  task_report(task_manager, "goto");

  waypoints.clear(); // clear waypoints so abort wont do anything

  autopilot_parms.goto_target = true;
  return run_flight(task_manager, autopilot_parms, n_wind);
}

static bool
test_null()
{
  GlidePolar glide_polar(fixed_two);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose)
    distance_counts();

  TaskEventsPrint default_events(verbose);

  TaskManager task_manager(default_events, waypoints);

  TaskBehaviour task_behaviour = task_manager.GetTaskBehaviour();
  task_behaviour.DisableAll();
  task_behaviour.enable_trace = false;
  task_manager.SetTaskBehaviour(task_behaviour);

  task_manager.SetGlidePolar(glide_polar);

  task_report(task_manager, "null");

  waypoints.clear(); // clear waypoints so abort wont do anything

  autopilot_parms.goto_target = true;
  return run_flight(task_manager, autopilot_parms, 0);
}

int
main(int argc, char** argv)
{
  // default arguments
  autopilot_parms.ideal();

  if (!parse_args(argc, argv))
    return 0;

  plan_tests(4);

  ok(test_abort(0), "abort", 0);
  ok(test_goto(0, 5, false), "goto", 0);
  ok(test_goto(0, 5, true), "goto with auto mc", 0);
  ok(test_null(), "null", 0);

  return exit_status();
}
