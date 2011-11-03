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

#include "harness_flight.hpp"
#include "harness_airspace.hpp"
#include "harness_waypoints.hpp"
#include "TaskEventsPrint.hpp"
#include <fstream>

extern double time_elapsed;
extern double calc_cruise_efficiency;
extern double calc_effective_mc;


bool test_speed_factor(int test_num, int n_wind) 
{
  // flying at opt speed should be minimum time flight!

  double te0, te1, te2;

  TestFlightResult result = test_flight(test_num, n_wind, 1.0);
  te0 = result.time_elapsed;

  result = test_flight(test_num, n_wind, 0.7);
  te1 = result.time_elapsed;
  // time of this should be higher than nominal
  ok(te0<te1, test_name("vopt slow or",test_num, n_wind), 0);

  result = test_flight(test_num, n_wind, 1.5);
  te2 = result.time_elapsed;
  // time of this should be higher than nominal
  ok(te0<te2, test_name("vopt fast or",test_num, n_wind), 0);

  bool retval = (te0<te1) && (te0<te2);
  if (verbose || !retval) {
    printf("# sf 0.8 time_elapsed_rat %g\n",te1/te0);
    printf("# sf 1.2 time_elapsed_rat %g\n",te2/te0);
  }
  return retval;
}


bool test_cruise_efficiency(int test_num, int n_wind) 
{
  // tests functionality of cruise efficiency calculations

  double ce0, ce1, ce2, ce3, ce4, ce5, ce6;

  autopilot_parms.ideal();

  TestFlightResult result = test_flight(test_num, n_wind);
  ce0 = result.calc_cruise_efficiency;

  // wandering
  autopilot_parms.realistic();
  result = test_flight(test_num, n_wind);
  ce1 = result.calc_cruise_efficiency;
  // cruise efficiency of this should be lower than nominal
  if (ce0<=ce1 || verbose) {
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);
  }
  ok (ce0>ce1, test_name("ce wandering",test_num, n_wind),0);

  // flying too slow
  autopilot_parms.ideal();
  result = test_flight(test_num, n_wind, 0.8);
  ce2 = result.calc_cruise_efficiency;
  // cruise efficiency of this should be lower than nominal
  if (ce0<=ce2 || verbose) {
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);
  }
  ok (ce0>ce2, test_name("ce speed slow",test_num, n_wind),0);

  // flying too fast
  autopilot_parms.ideal();
  result = test_flight(test_num, n_wind, 1.2);
  ce3 = result.calc_cruise_efficiency;
  // cruise efficiency of this should be lower than nominal
  if (ce0<=ce3 || verbose) {
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);
  }
  ok (ce0>ce3, test_name("ce speed fast",test_num, n_wind),0);

  // higher than expected cruise sink
  autopilot_parms.sink_factor = fixed(1.2);
  result = test_flight(test_num, n_wind);
  ce4 = result.calc_cruise_efficiency;
  if (ce0<=ce4 || verbose) {
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);
  }
  ok (ce0>ce4, test_name("ce high sink",test_num, n_wind),0);
  // cruise efficiency of this should be lower than nominal
  autopilot_parms.sink_factor = fixed(1.0);

  // slower than expected climb
  autopilot_parms.climb_factor = fixed(0.8);
  result = test_flight(test_num, n_wind);
  ce5 = result.calc_cruise_efficiency;
  if (ce0<=ce5 || verbose) {
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);
  }
  ok (ce0>ce5, test_name("ce slow climb",test_num, n_wind),0);
  // cruise efficiency of this should be lower than nominal
  autopilot_parms.climb_factor = fixed(1.0);

  // lower than expected cruise sink; 
  autopilot_parms.sink_factor = fixed(0.8);
  result = test_flight(test_num, n_wind);
  ce6 = result.calc_cruise_efficiency;
  if (ce0>=ce6 || verbose) {
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);
  }
  ok (ce0<ce6, test_name("ce low sink",test_num, n_wind),0);
  // cruise efficiency of this should be greater than nominal
  autopilot_parms.sink_factor = fixed(1.0);

  bool retval = (ce0>ce1) && (ce0>ce2) && (ce0>ce3) && (ce0>ce4) && (ce0>ce5)
    && (ce0<ce6);
  if (verbose || !retval) {
    printf("# ce nominal %g\n",ce0);
    printf("# ce wandering %g\n",ce1);
    printf("# ce speed slow %g\n",ce2);
    printf("# ce speed fast %g\n",ce3);
    printf("# ce high sink %g\n",ce4);
    printf("# ce slow climb %g\n",ce5);
    printf("# ce low sink %g\n",ce6);
  }
  return retval;
}

bool test_bestcruisetrack(int test_num, int n_wind)
{

  // tests whether following the cruise track which compensates for wind drift
  // produces routes that are more on track than if the route is allowed to drift
  // downwind during climbs

  // this test allows for a small error margin

  autopilot_parms.enable_bestcruisetrack = false;
  TestFlightResult result = test_flight(test_num, n_wind);
  double t0 = result.time_elapsed;

  autopilot_parms.enable_bestcruisetrack = true;
  result = test_flight(test_num, n_wind);
  double t1 = result.time_elapsed;
  autopilot_parms.enable_bestcruisetrack = false;

  bool fine = (t1/t0<1.01);
  ok(fine,test_name("faster flying with bestcruisetrack",test_num, n_wind),0);
  
  if (!fine || verbose) {
    printf("# time ratio %g\n", t1/t0);
  }
  return fine;
}


bool test_abort(int n_wind)
{
  GlidePolar glide_polar(fixed_two);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose) {
    distance_counts();
  }

  TaskEventsPrint default_events(verbose);

  TaskManager task_manager(default_events,
                           waypoints);

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


bool test_goto(int n_wind, unsigned id, bool auto_mc)
{
  GlidePolar glide_polar(fixed_two);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose) {
    distance_counts();
  }

  TaskEventsPrint default_events(verbose);

  TaskManager task_manager(default_events,
                           waypoints);


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


bool test_null()
{
  GlidePolar glide_polar(fixed_two);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose) {
    distance_counts();
  }

  TaskEventsPrint default_events(verbose);

  TaskManager task_manager(default_events,
                           waypoints);

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


bool test_effective_mc(int test_num, int n_wind) 
{
  // tests functionality of effective mc calculations

  double ce0, ce1, ce2, ce3, ce4, ce5, ce6;

  autopilot_parms.ideal();

  TestFlightResult result = test_flight(test_num, n_wind);
  ce0 = result.calc_effective_mc;

  // wandering
  autopilot_parms.realistic();
  result = test_flight(test_num, n_wind);
  ce1 = result.calc_effective_mc;
  // effective mc of this should be lower than nominal
  if (ce0<=ce1 || verbose) {
    printf("# calc effective mc %g\n", result.calc_effective_mc);
  }
  ok (ce0>ce1, test_name("emc wandering",test_num, n_wind),0);

  // flying too slow
  autopilot_parms.ideal();
  result = test_flight(test_num, n_wind, 0.8);
  ce2 = result.calc_effective_mc;
  // effective mc of this should be lower than nominal
  if (ce0<=ce2 || verbose) {
    printf("# calc effective mc %g\n", result.calc_effective_mc);
  }
  ok (ce0>ce2, test_name("emc speed slow",test_num, n_wind),0);

  // flying too fast
  autopilot_parms.ideal();
  result = test_flight(test_num, n_wind, 1.2);
  ce3 = result.calc_effective_mc;
  // effective mc of this should be lower than nominal
  if (ce0<=ce3 || verbose) {
    printf("# calc effective mc %g\n", result.calc_effective_mc);
  }
  ok (ce0>ce3, test_name("emc speed fast",test_num, n_wind),0);

  // higher than expected cruise sink
  autopilot_parms.sink_factor = fixed(1.2);
  result = test_flight(test_num, n_wind);
  ce4 = result.calc_effective_mc;
  if (ce0<=ce4 || verbose) {
    printf("# calc effective mc %g\n", result.calc_effective_mc);
  }
  ok (ce0>ce4, test_name("emc high sink",test_num, n_wind),0);
  // effective mc of this should be lower than nominal
  autopilot_parms.sink_factor = fixed(1.0);

  // slower than expected climb
  autopilot_parms.climb_factor = fixed(0.8);
  result = test_flight(test_num, n_wind);
  ce5 = result.calc_effective_mc;
  if (ce0<=ce5 || verbose) {
    printf("# calc effective mc %g\n", result.calc_effective_mc);
  }
  ok (ce0>ce5, test_name("emc slow climb",test_num, n_wind),0);
  // effective mc of this should be lower than nominal
  autopilot_parms.climb_factor = fixed(1.0);

  // lower than expected cruise sink; 
  autopilot_parms.sink_factor = fixed(0.8);
  result = test_flight(test_num, n_wind);
  ce6 = result.calc_effective_mc;
  if (ce0>=ce6 || verbose) {
    printf("# calc effective mc %g\n", result.calc_effective_mc);
  }
  ok (ce0<ce6, test_name("emc low sink",test_num, n_wind),0);
  // effective mc of this should be greater than nominal
  autopilot_parms.sink_factor = fixed(1.0);

  bool retval = (ce0>ce1) && (ce0>ce2) && (ce0>ce3) && (ce0>ce4) && (ce0>ce5)
    && (ce0<ce6);
  if (verbose || !retval) {
    printf("# emc nominal %g\n",ce0);
    printf("# emc wandering %g\n",ce1);
    printf("# emc speed slow %g\n",ce2);
    printf("# emc speed fast %g\n",ce3);
    printf("# emc high sink %g\n",ce4);
    printf("# emc slow climb %g\n",ce5);
    printf("# emc low sink %g\n",ce6);
  }
  return retval;
}


bool test_olc(int n_wind, Contests olc_type)
{
  GlidePolar glide_polar(fixed_two);
  Waypoints waypoints;
  setup_waypoints(waypoints);

  if (verbose) {
    distance_counts();
  }

  TaskEventsPrint default_events(verbose);

  TaskManager task_manager(default_events,
                           waypoints);

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
