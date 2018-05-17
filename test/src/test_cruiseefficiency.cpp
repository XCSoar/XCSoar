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

static bool
test_cruise_efficiency(int test_num, int n_wind)
{
  // tests functionality of cruise efficiency calculations

  double ce0, ce1, ce2, ce3, ce4, ce5, ce6;

  autopilot_parms.SetIdeal();

  TestFlightResult result = test_flight(test_num, n_wind);
  ce0 = result.calc_cruise_efficiency;

  // wandering
  autopilot_parms.SetRealistic();
  result = test_flight(test_num, n_wind);
  ce1 = result.calc_cruise_efficiency;
  // cruise efficiency of this should be lower than nominal
  if (ce0 <= ce1 || verbose)
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);

  ok(ce0 > ce1, GetTestName("ce wandering", test_num, n_wind), 0);

  // flying too slow
  autopilot_parms.SetIdeal();
  result = test_flight(test_num, n_wind, 0.8);
  ce2 = result.calc_cruise_efficiency;
  // cruise efficiency of this should be lower than nominal
  if (ce0 <= ce2 || verbose)
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);

  ok(ce0 > ce2, GetTestName("ce speed slow", test_num, n_wind), 0);

  // flying too fast
  autopilot_parms.SetIdeal();
  result = test_flight(test_num, n_wind, 1.2);
  ce3 = result.calc_cruise_efficiency;
  // cruise efficiency of this should be lower than nominal
  if (ce0 <= ce3 || verbose)
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);

  ok(ce0 > ce3, GetTestName("ce speed fast", test_num, n_wind), 0);

  // higher than expected cruise sink
  autopilot_parms.sink_factor = 1.2;
  result = test_flight(test_num, n_wind);
  ce4 = result.calc_cruise_efficiency;
  if (ce0 <= ce4 || verbose)
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);

  ok(ce0 > ce4, GetTestName("ce high sink", test_num, n_wind), 0);
  // cruise efficiency of this should be lower than nominal
  autopilot_parms.sink_factor = 1.0;

  // slower than expected climb
  autopilot_parms.climb_factor = 0.8;
  result = test_flight(test_num, n_wind);
  ce5 = result.calc_cruise_efficiency;
  if (ce0 <= ce5 || verbose)
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);

  ok(ce0 > ce5, GetTestName("ce slow climb", test_num, n_wind), 0);
  // cruise efficiency of this should be lower than nominal
  autopilot_parms.climb_factor = 1.0;

  // lower than expected cruise sink;
  autopilot_parms.sink_factor = 0.8;
  result = test_flight(test_num, n_wind);
  ce6 = result.calc_cruise_efficiency;
  if (ce0 >= ce6 || verbose)
    printf("# calc cruise efficiency %g\n", result.calc_cruise_efficiency);

  ok(ce0 < ce6, GetTestName("ce low sink", test_num, n_wind), 0);
  // cruise efficiency of this should be greater than nominal
  autopilot_parms.sink_factor = 1.0;

  bool retval = (ce0 > ce1) && (ce0 > ce2) && (ce0 > ce3) && (ce0 > ce4)
      && (ce0 > ce5) && (ce0 < ce6);
  if (verbose || !retval) {
    printf("# ce nominal %g\n", ce0);
    printf("# ce wandering %g\n", ce1);
    printf("# ce speed slow %g\n", ce2);
    printf("# ce speed fast %g\n", ce3);
    printf("# ce high sink %g\n", ce4);
    printf("# ce slow climb %g\n", ce5);
    printf("# ce low sink %g\n", ce6);
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
  plan_tests(6);

  // tests whether cruise efficiency is calculated correctly
  test_cruise_efficiency(3, i);

  return exit_status();
}
