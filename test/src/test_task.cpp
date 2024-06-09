// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_task.hpp"
#include "harness_waypoints.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "test_debug.hpp"

extern "C" {
#include "tap.h"
}

static bool
distance_is_equal(double actual, double expected) {
  constexpr double TOLERANCE_PERCENT = 0.5;
  return std::abs(actual/expected - 1.0) < TOLERANCE_PERCENT/100;
}

static void
assert_distances(const TaskManager &task_manager,
                 double expected_max,
                 double expected_min) 
{
  auto stats = task_manager.GetStats();
  ok( distance_is_equal( stats.distance_max, expected_max ),
      "task distance max: got %.2f, expected %.2f",
      stats.distance_max, expected_max );

  ok( distance_is_equal( stats.distance_min, expected_min ),
      "task distance min: got %.2f, expected %.2f",
      stats.distance_min, expected_min );

  // before starting to fly the task, flyable maximum and total maximum are the same
  ok( distance_is_equal( stats.distance_max_total, stats.distance_max ),
      "task distance max total: got %.2f, expected %.2f",
      stats.distance_max_total, stats.distance_max_total );
}

/**
 * Check AAT min & max distance algorithm. Compare to values calculated
 * in an external application for independent reference. 
 * Test task:
 *   - Start:  lat 0°, long 0°, cyl radius 1 km
 *   - TP 1:   lat 1°, long 0°, cyl radius 30 km
 *   - TP 2:   lat 1°, long 1°, cyl radius 40 km
 *   - Finish: lat 0°, long 0°, cyl radius 1 km  
*/
static void 
assert_aat_distances(const TaskManager &task_manager) 
{
  constexpr double EXPECTED_MAX_DIST = 496300.0;
  constexpr double EXPECTED_MIN_DIST = 265100.0;

  assert_distances(task_manager, EXPECTED_MAX_DIST, EXPECTED_MIN_DIST);
}

/**
 * Check AAT min & max distance algorithm. Compare to values calculated
 * in an external application for independent reference. 
 * Test task:
 *   - Start:  lat 0°   long 0°,   line length 1 km
 *   - TP 1:   lat 1°   long 0°,   AST cylinder radius 500 m
 *   - TP 2:   lat 1°   long 1°,   cyl radius 30 km
 *   - TP 3:   lat 0.5° long 0.8°  cyl radius 500 m
 *   - TP 4:   lat 0°   long 1°    cyl radius 30 km
 *   - Finish: lat 0°   long 0°    line lenght 1 km
*/
static void 
assert_mixed_task_distances(const TaskManager &task_manager) 
{
  constexpr double EXPECTED_MAX_DIST = 559300;
  constexpr double EXPECTED_MIN_DIST = 364000;

  assert_distances(task_manager, EXPECTED_MAX_DIST, EXPECTED_MIN_DIST);
}

int main(int argc, char** argv)
{
  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  static constexpr unsigned NUM_RANDOM = 50;
  static constexpr unsigned NUM_TYPE_MANIPS = 50;
  plan_tests(8+NUM_TYPE_MANIPS+NUM_TASKS+2+6+NUM_RANDOM);
  
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

    if( i==0 )
      assert_mixed_task_distances(task_manager);
    if( i==2 )
      assert_aat_distances(task_manager);
  }

  for (unsigned i=0; i<NUM_RANDOM; i++) {
    TaskManager task_manager(task_behaviour, waypoints);
    task_manager.SetGlidePolar(glide_polar);
    ok(test_task(task_manager, waypoints, 7),GetTestName("construction",7,0),0);
  }

  return exit_status();
}

