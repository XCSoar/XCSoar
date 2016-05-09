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

#include "TestUtil.hpp"
#include <stdio.h>

#include "Engine/ThermalBand/ThermalEncounterBand.hpp"
#include "Engine/ThermalBand/ThermalEncounterCollection.hpp"

int verbose = 1;

static void report(ThermalBand& band, const char* title)
{
  if (!verbose)
    return;

  printf("# band %s\n", title);
  const unsigned s = band.size();
  for (unsigned i=0; i<s; ++i) {
    const ThermalSlice& s = band.GetSlice(i);
    printf("# %g %g %g %g %g %g\n", band.GetSliceCenter(i), s.w_n, s.n, s.w_t, s.dt, s.time);
  }
}

static void simulate_climb(ThermalEncounterBand& band,
                           double& t, double &h,
                           const double td,
                           const double dt,
                           const double w0,
                           const double w1)
{
  const double ts = t;
  const double te = ts+td;
  for (; t< te; t+= dt) {
    double f = (t-ts)/td;
    h += (w0*(1-f)+w1*(f))*dt;
    band.AddSample(t, h);
  }
}

static void thermal_encounter_test(const double telapsed,
                                   const double dt,
                                   const double w0,
                                   const double w1)
{
  ThermalEncounterBand band;
  double t=0;
  double h=123;
  const double w_tol = 0.1*(w0+w1)/2;
  const double h_tol = 25;

  band.Reset();

  simulate_climb(band, t, h, telapsed, dt, w0, w1);

  ok1(band.Valid());
  ok1(fabs(band.GetTimeElapsed()-telapsed)< dt);
  if (verbose) printf("# Time elapsed %g %g\n", band.GetTimeElapsed(), telapsed);

  const unsigned s = band.size();
  ok1(fabs(band.GetSliceCenter(s-1)-h)<h_tol);
  if (verbose) printf("# Ceiling %g %g\n", band.GetSliceCenter(s-1), h);

  ok1(fabs(band.GetSlice(0).w_n-w0) < w_tol);
  ok1(fabs(band.GetSlice(0).w_t-w0) < w_tol);

  ok1(fabs(band.GetSlice(s-1).w_n-w1) < w_tol);
  ok1(fabs(band.GetSlice(s-1).w_t-w1) < w_tol);

  report(band,"enc");
}

static void thermal_collection_test(const double telapsed,
                                    const double dt,
                                    const double w0,
                                    const double w1)
{
  ThermalEncounterCollection col;

  ThermalEncounterBand band;

  band.Reset();

  const double h0 = 123;
  const double n_tol = 0.001;
  const double w_tol = 0.1*(w0+w1)/2;
  double t=0;
  double h= h0;
  //const double h_tol = 25;

  printf("# 1------\n");
  col.Reset();

  simulate_climb(band, t, h, telapsed, dt, w0, w0);
  report(band,"enc");
  col.Merge(band);
  ok1(col.Valid());
  report(col,"col");

  printf("# 2------\n");
  // same climb, later
  band.Reset();
  t += 100; h = h0;
  simulate_climb(band, t, h, telapsed, dt, w0, w0);
  report(band,"enc");
  col.Merge(band);
  report(col,"col");

  // expect two climbs at base and ceiling
  ok1(fabs(col.GetSlice(0).n-2)< n_tol);
  ok1(fabs(col.GetSlice(col.size()-2).n-2)< n_tol);

  printf("# 3------\n");
  // different climb, half way up
  band.Reset();
  t += 100; h = h0+45;
  simulate_climb(band, t, h, 35, dt, w1, w1);
  report(band,"enc");
  col.Merge(band);
  report(col,"col");

  // expect two climbs at base and ceiling, 3 in middle
  ok1(fabs(col.GetSlice(0).n-2)< n_tol);
  ok1(fabs(col.GetSlice(col.size()-2).n-2)< n_tol);
  ok1(fabs(col.GetSlice(col.size()/2).n-3)< n_tol);

  const double we = (w0*2+w1)/3;

  // expect strengths of climbs at ends w0, and middle we
  ok1(fabs(col.GetSlice(0).w_n-w0) < w_tol);
  ok1(fabs(col.GetSlice(col.size()-2).w_n-w0) < w_tol);
  ok1(fabs(col.GetSlice(col.size()/2).w_n-we) < w_tol);
}


int main(int argc, char** argv) {
  const int num_encounter_tests = 8;
  const int num_collection_tests = 2;
  plan_tests(7*num_encounter_tests + 9*num_collection_tests);

  // test different thermal strengths
  thermal_encounter_test(100,1,1,1);
  thermal_encounter_test(100,1,10,10);
  thermal_encounter_test(100,1,0.1,0.1);

  // test forcing decimation
  thermal_encounter_test(1000,1,1,1);

  // test thermal increasing strength, for different durations
  thermal_encounter_test(100,1,1,3);
  thermal_encounter_test(300,1,1,3);

  // test thermal decreasing strength
  thermal_encounter_test(300,1,3,1);

  // test thermal decreasing strength
  thermal_encounter_test(300,1,3,1);

  // test collection, same strength
  thermal_collection_test(100,1,1,1);

  // test collection, different strength
  thermal_collection_test(100,1,1,0.5);

  return exit_status();
}
