// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_flight.hpp"
#include "harness_wind.hpp"
#include "test_debug.hpp"

extern "C" {
#include "tap.h"
}

static bool
test_automc(int test_num, int n_wind)
{
  // test whether flying by automc (starting above final glide)
  // arrives home faster than without

  TestFlightResult result = test_flight(test_num, n_wind, 1.0, false);
  const FloatDuration t0 = result.time_elapsed;

  result = test_flight(test_num, n_wind, 1.0, true);
  const FloatDuration t1 = result.time_elapsed;

  bool fine = (t1 / t0 < 1.015);
  if (!fine || verbose)
    printf("# time ratio %g\n", t1 / t0);

  ok(fine, GetTestName("faster with auto mc on", test_num, n_wind), 0);

  return fine;
}

int
main(int argc, char** argv)
{
  // default arguments
  autopilot_parms.SetIdeal();

  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  plan_tests(NUM_WIND);

  for (unsigned i=0; i<NUM_WIND; i++) {
    test_automc(5,i);
  }
  return exit_status();
}
