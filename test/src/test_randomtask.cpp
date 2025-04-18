// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_flight.hpp"
#include "harness_wind.hpp"
#include "test_debug.hpp"

extern "C" {
#include "tap.h"
}

int
main(int argc, char** argv)
{
  // default arguments
  autopilot_parms.SetIdeal();

  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  static constexpr unsigned NUM_TESTS = 2;

  plan_tests(NUM_TESTS);

  for (unsigned j=0; j<NUM_TESTS; j++) {
    unsigned i = rand()%NUM_WIND;

    if (j+1==NUM_TESTS) {
      verbose=1;
    }
    ok (test_flight_times(7,i), GetTestName("flight times",7,i),0);
  }
  return exit_status();
}
