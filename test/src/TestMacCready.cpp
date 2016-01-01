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

#include "Geo/SpeedVector.hpp"
#include "Engine/GlideSolvers/GlideSettings.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"

#include "TestUtil.hpp"

static GlideSettings glide_settings;
static GlidePolar glide_polar(0);

static void
Test(const double distance, const double altitude, const SpeedVector wind)
{
  const GeoVector vector(distance, Angle::Zero());
  const GlideState state(vector,
                         2000, 2000 + altitude,
                         wind);
  const GlideResult result =
    MacCready::Solve(glide_settings, glide_polar, state);

  const double ld_ground = glide_polar.GetLDOverGround(vector.bearing, wind);

  const double mc = glide_polar.GetMC();
  const double v_climb_progress = mc * ld_ground - state.head_wind;

  const double initial_glide_distance = state.altitude_difference * ld_ground;
  if (initial_glide_distance >= distance ||
      (mc <= 0 && v_climb_progress <= 0)) {
    /* reachable by pure glide */
    ok1(result.validity == GlideResult::Validity::OK);

    const double best_speed =
      glide_polar.GetBestGlideRatioSpeed(state.head_wind);
    const double best_sink = glide_polar.SinkRate(best_speed);
    const double ld_ground2 = mc > 0
      ? ld_ground
      : (best_speed - state.head_wind) / best_sink;

    const double height_glide = distance / ld_ground2;
    const double height_climb = 0;
    const double altitude_difference = altitude - height_glide;

    ok1(equals(result.head_wind, wind.norm));
    ok1(equals(result.vector.distance, distance));
    ok1(equals(result.height_climb, height_climb));
    ok1(equals(result.height_glide, height_glide));
    ok1(equals(result.altitude_difference, altitude_difference));
    return;
  }

  if (v_climb_progress <= 0) {
    /* excessive wind */
    ok1(result.validity == GlideResult::Validity::WIND_EXCESSIVE);
    return;
  }

  /*
  const double drifted_distance = (distance - initial_glide_distance)
    * state.head_wind / v_climb_progress;
    */
  const double drifted_height_climb = (distance - initial_glide_distance)
    * mc / v_climb_progress;
  const double drifted_height_glide =
    drifted_height_climb + state.altitude_difference;

  const double height_glide = drifted_height_glide;
  const double altitude_difference = altitude - height_glide;
  const double height_climb = drifted_height_climb;

  const double time_climb = height_climb / mc;
  const double time_glide = height_glide / glide_polar.GetSBestLD();
  const double time_elapsed = time_climb + time_glide;

  /* more tolerance with strong wind because this unit test doesn't
     optimise pure glide */
  const int accuracy = altitude > 0 && wind.norm > 0
    ? (wind.norm > 5 ? 5 : 10)
    : ACCURACY;

  ok1(result.validity == GlideResult::Validity::OK);
  ok1(equals(result.head_wind, wind.norm));
  ok1(equals(result.vector.distance, distance));
  ok1(equals(result.height_climb, height_climb, accuracy));
  ok1(equals(result.height_glide, height_glide, accuracy));
  ok1(equals(result.altitude_difference, altitude_difference, accuracy));
  ok1(equals(result.time_elapsed, time_elapsed, accuracy));
}

static void
TestWind(const SpeedVector &wind)
{
  Test(10000, -200, wind);
  Test(10000, -100, wind);
  Test(10000, 0, wind);
  Test(10000, 100, wind);
  Test(10000, 200, wind);

  Test(1000, -500, wind);
  Test(1000, -100, wind);
  Test(1000, 0, wind);
  Test(1000, 100, wind);
  Test(1000, 500, wind);
  Test(100000, -1000, wind);
  Test(100000, 4000, wind);
}

static void
TestAll()
{
  TestWind(SpeedVector(Angle::Zero(), 0));
  TestWind(SpeedVector(Angle::Zero(), 2));
  TestWind(SpeedVector(Angle::Zero(), 5));
  TestWind(SpeedVector(Angle::Zero(), 10));
  TestWind(SpeedVector(Angle::Zero(), 15));
  TestWind(SpeedVector(Angle::Zero(), 30));
}

int main(int argc, char **argv)
{
  plan_tests(2095);

  glide_settings.SetDefaults();

  TestAll();

  glide_polar.SetMC(0.1);
  TestAll();

  glide_polar.SetMC(1);
  TestAll();

  glide_polar.SetMC(4);
  TestAll();

  glide_polar.SetMC(10);
  TestAll();

  return exit_status();
}
