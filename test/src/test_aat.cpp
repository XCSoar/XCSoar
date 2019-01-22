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
test_aat(int test_num, int n_wind)
{
  // test whether flying to targets in an AAT task produces
  // elapsed (finish) times equal to desired time with 1.5% tolerance

  TestFlightResult result = test_flight(test_num, n_wind);
  bool fine = result.result;
  double min_time = (double)aat_min_time(test_num) + 300.0;
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

#define NUM_FLIGHT 2

  plan_tests(NUM_FLIGHT*2);

  for (int i=0; i<NUM_FLIGHT; i++) {
    unsigned k = rand()%NUM_WIND;
    ok (test_aat(2,k), GetTestName("target ",2,k),0);
  }
  for (int i=0; i<NUM_FLIGHT; i++) {
    unsigned k = rand()%NUM_WIND;
    ok (test_aat(0,k), GetTestName("target ",0,k),0);
  }
  return exit_status();
}
