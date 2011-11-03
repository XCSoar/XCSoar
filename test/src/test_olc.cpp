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
#include "harness_airspace.hpp"

static bool
test_olc(int n_wind, Contests olc_type)
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
  task_behaviour.enable_olc = true;
  if (!verbose)
    task_behaviour.enable_trace = false;
  task_manager.SetTaskBehaviour(task_behaviour);

  task_manager.SetGlidePolar(glide_polar);
  test_task(task_manager, waypoints, 1);

  waypoints.clear(); // clear waypoints so abort wont do anything

  autopilot_parms.goto_target = true;
  return run_flight(task_manager, autopilot_parms, n_wind);
}

int
main(int argc, char** argv)
{
  // default arguments
  autopilot_parms.realistic();
  autopilot_parms.start_alt = fixed(400);

  if (!parse_args(argc, argv))
    return 0;

  plan_tests(3);

  ok(test_olc(0, OLC_Sprint), "olc sprint", 0);
  ok(test_olc(0, OLC_Classic), "olc classic", 0);
  ok(test_olc(0, OLC_FAI), "olc fai", 0);

  return exit_status();
}

