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

static constexpr unsigned NUM_TERRAIN = 5;

  plan_tests(NUM_TERRAIN);

  for (unsigned j=0; j<NUM_TERRAIN; j++) {
    terrain_height = (int)((800)*(j+1)/(NUM_TERRAIN*1.0));

    unsigned i = rand()%NUM_WIND;
    ok (test_flight_times(1,i), GetTestName("high terrain",7,i),0);
  }
  return exit_status();
}
