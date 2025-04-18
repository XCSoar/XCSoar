// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_flight.hpp"
#include "test_debug.hpp"

extern "C" {
#include "tap.h"
}

static bool
test_airspace(const unsigned n_airspaces)
{
  TestFlightComponents components;
  components.airspaces = new Airspaces;
  setup_airspaces(*components.airspaces, GeoPoint(Angle::Degrees(0.5), Angle::Degrees(0.5)), n_airspaces);
  bool fine = test_flight(components, 4, 0);
  delete components.airspaces;
  return fine;
}

int main(int argc, char** argv) 
{
  // default arguments
  autopilot_parms.SetIdeal();

  if (!ParseArgs(argc,argv)) {
    return 0;
  }

  plan_tests(3);

  ok(test_airspace(20),"airspace 20",0);
  ok(test_airspace(100),"airspace 100",0);
  
  Airspaces airspaces;
  setup_airspaces(airspaces, GeoPoint(Angle::Zero(), Angle::Zero()), 20);
  ok(test_airspace_extra(airspaces),"airspace extra",0);

  return exit_status();
}
