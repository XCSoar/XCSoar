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
