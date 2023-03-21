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

  plan_tests(NUM_TASKS);

  for (unsigned j=0; j<NUM_TASKS; j++) {
    unsigned k = rand()%NUM_WIND;
    ok (test_flight_times(j,k), GetTestName("flight times",j,k),0);
  }
  return exit_status();
}
