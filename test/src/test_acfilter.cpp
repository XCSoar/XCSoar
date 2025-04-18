// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_flight.hpp"
#include "Util/AircraftStateFilter.hpp"
#include "test_debug.hpp"

extern "C" {
#include "tap.h"
}

int main(int argc, char** argv) {
  // default arguments
  autopilot_parms.SetRealistic();

  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  AircraftState dummy;
  TestFlightComponents components;
  components.aircraft_filter = new AircraftStateFilter(120);
  components.aircraft_filter->Reset(dummy);

  plan_tests(1);
  ok(test_flight(components, 1,0,1.0,true),"basic flight test",0);

  delete components.aircraft_filter;

  return exit_status();
}

/*
  use get_speed() and get_climb_rate() as inputs to AirspacePerformanceModel()
  as a potential constructor for short range.

  look for intersections along this line (possibly with vert speed margins)

  so, four airspace warning queries:
   -- inside
   -- intersection in short duration (next 10 seconds):  warning
   -- intersection in medium duration (based on filter), 1 minute?
   -- long duration based on task track (display only highlighted)

*/
