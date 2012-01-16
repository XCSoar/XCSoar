/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Engine/Navigation/SpeedVector.hpp"
#include "Engine/GlideSolvers/GlideSettings.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"

#ifdef FIXED_MATH
#define ACCURACY 1000
#endif

#include "TestUtil.hpp"

static GlideSettings glide_settings;
static GlidePolar glide_polar(fixed_zero);

static void
Test(const fixed distance, const fixed altitude, const SpeedVector wind)
{
  const GeoVector vector(distance, Angle::Zero());
  const GlideState state(vector,
                         fixed(2000), fixed(2000) + altitude,
                         wind);
  const GlideResult result =
    MacCready::Solve(glide_settings, glide_polar, state);

  const fixed ld_ground = glide_polar.GetLDOverGround(vector.bearing, wind);

  const fixed mc = glide_polar.GetMC();
  const fixed v_climb_progress = mc * ld_ground - state.head_wind;

  const fixed initial_climb_distance = state.altitude_difference * ld_ground;
  if (-initial_climb_distance >= distance ||
      (!positive(mc) && !positive(v_climb_progress))) {
    /* reachable by pure glide */
    ok1(result.validity == GlideResult::Validity::OK);

    const fixed height_glide = distance / ld_ground;
    const fixed height_climb = fixed_zero;
    const fixed altitude_difference = altitude - height_glide;

    /* more tolerance with strong wind because this unit test doesn't
       optimise pure glide */
    const int accuracy = wind.norm > fixed_ten
      ? 5 : (wind.norm > fixed_one ? 10 : ACCURACY);

    ok1(equals(result.head_wind, wind.norm));
    ok1(equals(result.vector.distance, distance));
    ok1(equals(result.height_climb, height_climb, accuracy));
    ok1(equals(result.height_glide, height_glide, accuracy));
    ok1(equals(result.altitude_difference, altitude_difference, accuracy));
    return;
  }

  if (!positive(v_climb_progress)) {
    /* excessive wind */
    ok1(result.validity == GlideResult::Validity::WIND_EXCESSIVE);
    return;
  }

  /*
  const fixed drifted_distance = (distance - initial_climb_distance)
    * state.head_wind / v_climb_progress;
    */
  const fixed drifted_height_climb = (distance - initial_climb_distance)
    * mc / v_climb_progress;
  const fixed drifted_height_glide =
    drifted_height_climb + state.altitude_difference;

  const fixed height_glide = drifted_height_glide;
  const fixed altitude_difference = altitude - height_glide;
  const fixed height_climb = drifted_height_climb;

  const fixed time_climb = height_climb / mc;
  const fixed time_glide = height_glide / glide_polar.GetSBestLD();
  const fixed time_elapsed = time_climb + time_glide;

  /* more tolerance with strong wind because this unit test doesn't
     optimise pure glide */
  const int accuracy = positive(altitude) && positive(wind.norm)
    ? (wind.norm > fixed(5) ? 5 : 10)
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
  Test(fixed(10000), fixed(-200), wind);
  Test(fixed(10000), fixed(-100), wind);
  Test(fixed(10000), fixed_zero, wind);
  Test(fixed(10000), fixed(100), wind);
  Test(fixed(10000), fixed(200), wind);
}

static void
TestAll()
{
  TestWind(SpeedVector(Angle::Zero(), fixed_zero));
  TestWind(SpeedVector(Angle::Zero(), fixed(2)));
  TestWind(SpeedVector(Angle::Zero(), fixed(5)));
  TestWind(SpeedVector(Angle::Zero(), fixed(10)));
  TestWind(SpeedVector(Angle::Zero(), fixed(15)));
}

int main(int argc, char **argv)
{
  plan_tests(760);

  glide_settings.SetDefaults();

  TestAll();

  glide_polar.SetMC(fixed(0.1));
  TestAll();

  glide_polar.SetMC(fixed_one);
  TestAll();

  glide_polar.SetMC(fixed_four);
  TestAll();

  glide_polar.SetMC(fixed_ten);
  TestAll();

  return exit_status();
}
