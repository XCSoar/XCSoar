// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_flight.hpp"
#include "harness_wind.hpp"
#include "test_debug.hpp"

extern "C" {
#include "tap.h"
}

static bool
test_bestcruisetrack(int test_num, int n_wind)
{

  // tests whether following the cruise track which compensates for wind drift
  // produces routes that are more on track than if the route is allowed to drift
  // downwind during climbs

  // this test allows for a small error margin

  autopilot_parms.enable_bestcruisetrack = false;
  TestFlightResult result = test_flight(test_num, n_wind);
  const auto t0 = result.time_elapsed;

  autopilot_parms.enable_bestcruisetrack = true;
  result = test_flight(test_num, n_wind);
  const auto t1 = result.time_elapsed;
  autopilot_parms.enable_bestcruisetrack = false;

  bool fine = (t1 / t0 < 1.01);
  ok(fine, GetTestName("faster flying with bestcruisetrack", test_num, n_wind),
     0);

  if (!fine || verbose)
    printf("# time ratio %g\n", t1 / t0);

  return fine;
}

int
main(int argc, char** argv)
{
  // default arguments
  autopilot_parms.SetIdeal();

  if (!ParseArgs(argc, argv))
    return 0;

  unsigned i = rand() % (NUM_WIND - 1) + 1;
  plan_tests(1);

  test_bestcruisetrack(1, i);
  return exit_status();
}
