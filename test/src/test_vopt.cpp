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
test_speed_factor(int test_num, int n_wind)
{
  // flying at opt speed should be minimum time flight!

  double te0, te1, te2;

  TestFlightResult result = test_flight(test_num, n_wind, 1.0);
  te0 = result.time_elapsed;

  result = test_flight(test_num, n_wind, 0.7);
  te1 = result.time_elapsed;
  // time of this should be higher than nominal
  ok(te0 < te1, GetTestName("vopt slow or", test_num, n_wind), 0);

  result = test_flight(test_num, n_wind, 1.5);
  te2 = result.time_elapsed;
  // time of this should be higher than nominal
  ok(te0 < te2, GetTestName("vopt fast or", test_num, n_wind), 0);

  bool retval = (te0 < te1) && (te0 < te2);
  if (verbose || !retval) {
    printf("# sf 0.8 time_elapsed_rat %g\n", te1 / te0);
    printf("# sf 1.2 time_elapsed_rat %g\n", te2 / te0);
  }
  return retval;
}

int
main(int argc, char** argv)
{
  // default arguments
  autopilot_parms.SetIdeal();

  if (!ParseArgs(argc, argv))
    return 0;

  unsigned i = rand() % NUM_WIND;
  plan_tests(2);

  // tests whether flying at VOpt for OR task is optimal
  test_speed_factor(3, i);
  return exit_status();
}
