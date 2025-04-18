// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_flight.hpp"
#include "harness_wind.hpp"
#include "test_debug.hpp"

extern "C" {
#include "tap.h"
}

static bool
test_speed_factor(int test_num, int n_wind)
{
  // flying at opt speed should be minimum time flight!

  TestFlightResult result = test_flight(test_num, n_wind, 1.0);
  const FloatDuration te0 = result.time_elapsed;

  result = test_flight(test_num, n_wind, 0.7);
  const auto te1 = result.time_elapsed;
  // time of this should be higher than nominal
  ok(te0 < te1, GetTestName("vopt slow or", test_num, n_wind), 0);

  result = test_flight(test_num, n_wind, 1.5);
  const auto te2 = result.time_elapsed;
  // time of this should be higher than nominal
  ok(te0 < te2, GetTestName("vopt fast or", test_num, n_wind), 0);

  bool retval = (te0 < te1) && (te0 < te2);
  if (verbose || !retval) {
    printf("# sf 0.8 time_elapsed_rat %g\n", te1 / te0);
    printf("# sf 1.2 time_elapsed_rat %g\n", te2 / te0);
  }
  return retval;
}

int
main(int argc, char** argv)
{
  // default arguments
  autopilot_parms.SetIdeal();

  if (!ParseArgs(argc, argv))
    return 0;

  unsigned i = rand() % NUM_WIND;
  plan_tests(2);

  // tests whether flying at VOpt for OR task is optimal
  test_speed_factor(3, i);
  return exit_status();
}
