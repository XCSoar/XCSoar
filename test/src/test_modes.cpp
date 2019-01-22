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

#include "TaskEventsPrint.hpp"
#include "harness_flight.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "test_debug.hpp"

extern "C" {
#include "tap.h"
}

static bool
test_abort(int n_wind)
{
  GlidePolar glide_polar(2);
  Waypoints waypoints;
  SetupWaypoints(waypoints);

  if (verbose)
    PrintDistanceCounts();

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();
  task_behaviour.DisableAll();

  TaskManager task_manager(task_behaviour, waypoints);

  TaskEventsPrint default_events(verbose);
  task_manager.SetTaskEvents(default_events);

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
  GlidePolar glide_polar(2);
  Waypoints waypoints;
  SetupWaypoints(waypoints);

  if (verbose)
    PrintDistanceCounts();

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();
  task_behaviour.DisableAll();
  task_behaviour.auto_mc = auto_mc;

  TaskManager task_manager(task_behaviour, waypoints);

  TaskEventsPrint default_events(verbose);
  task_manager.SetTaskEvents(default_events);

  task_manager.SetGlidePolar(glide_polar);

  test_task(task_manager, waypoints, 1);

  task_manager.DoGoto(waypoints.LookupId(id));
  task_report(task_manager, "goto");

  waypoints.Clear(); // clear waypoints so abort wont do anything

  autopilot_parms.goto_target = true;
  return run_flight(task_manager, autopilot_parms, n_wind);
}

static bool
test_null()
{
  GlidePolar glide_polar(2);
  Waypoints waypoints;
  SetupWaypoints(waypoints);

  if (verbose)
    PrintDistanceCounts();

  TaskBehaviour task_behaviour;
  task_behaviour.SetDefaults();
  task_behaviour.DisableAll();

  TaskManager task_manager(task_behaviour, waypoints);

  TaskEventsPrint default_events(verbose);
  task_manager.SetTaskEvents(default_events);

  task_manager.SetGlidePolar(glide_polar);

  task_report(task_manager, "null");

  waypoints.Clear(); // clear waypoints so abort wont do anything

  autopilot_parms.goto_target = true;
  return run_flight(task_manager, autopilot_parms, 0);
}

int
main(int argc, char** argv)
{
  // default arguments
  autopilot_parms.SetIdeal();

  if (!ParseArgs(argc, argv))
    return 0;

  plan_tests(4);

  ok(test_abort(0), "abort", 0);
  ok(test_goto(0, 5, false), "goto", 0);
  ok(test_goto(0, 5, true), "goto with auto mc", 0);
  ok(test_null(), "null", 0);

  return exit_status();
}
