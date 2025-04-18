// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_flight.hpp"
#include "harness_wind.hpp"
#include "test_debug.hpp"

extern "C" {
#include "tap.h"
}

using namespace std::chrono;

static bool
test_aat(int test_num, int n_wind)
{
  // test whether flying to targets in an AAT task produces
  // elapsed (finish) times equal to desired time with 1.5% tolerance

  TestFlightResult result = test_flight(test_num, n_wind);
  bool fine = result.result;
  FloatDuration min_time = FloatDuration{aat_min_time(test_num)} + minutes{5};
  // 300 second offset is default 5 minute margin provided in TaskBehaviour

  const double t_ratio = fabs(result.time_elapsed / min_time - 1.0);
  fine &= (t_ratio < 0.015);
  if (!fine || verbose)
    printf("# time ratio error (elapsed/target) %g\n", t_ratio);

  return fine;
}

int main(int argc, char** argv) 
{
  // default arguments
  autopilot_parms.SetIdeal();

  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  static constexpr unsigned NUM_FLIGHT = 2;

  plan_tests(NUM_FLIGHT*2);

  for (unsigned i=0; i<NUM_FLIGHT; i++) {
    unsigned k = rand()%NUM_WIND;
    ok (test_aat(2,k), GetTestName("target ",2,k),0);
  }
  for (unsigned i=0; i<NUM_FLIGHT; i++) {
    unsigned k = rand()%NUM_WIND;
    ok (test_aat(0,k), GetTestName("target ",0,k),0);
  }
  return exit_status();
}
