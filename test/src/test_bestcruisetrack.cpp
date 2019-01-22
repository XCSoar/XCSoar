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
  double t0 = result.time_elapsed;

  autopilot_parms.enable_bestcruisetrack = true;
  result = test_flight(test_num, n_wind);
  double t1 = result.time_elapsed;
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
