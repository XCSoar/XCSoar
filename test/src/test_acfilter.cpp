/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
