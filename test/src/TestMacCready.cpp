// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

  const FloatDuration time_climb{height_climb / mc};
  const FloatDuration time_glide{height_glide / glide_polar.GetSBestLD()};
  const FloatDuration time_elapsed = time_climb + time_glide;

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
TestArrivesAtBestGlide()
{
  GlidePolar polar(0);
  polar.SetMC(4);

  const GeoVector vector(60000, Angle::Zero());
  const SpeedVector wind(Angle::Zero(), 0);
  const GlideState probe(vector, 0, 8000, wind);

  GlidePolar best = polar;
  best.SetMC(0);
  const GlideResult at_mc =
    MacCready(glide_settings, polar).SolveStraight(probe);
  const GlideResult at_best =
    MacCready(glide_settings, best).SolveGlide(probe, best.GetVBestLD());
  ok1(at_mc.IsOk() && at_best.IsOk());
  ok1(at_best.pure_glide_altitude_difference >
      at_mc.pure_glide_altitude_difference + 10);

  const double nav_between =
    8000 - at_best.pure_glide_altitude_difference + 20;
  const GlideState between(vector, 0, nav_between, wind);
  const GlideResult mc_between =
    MacCready(glide_settings, polar).SolveStraight(between);
  ok1(mc_between.IsOk() &&
      mc_between.pure_glide_altitude_difference < 0);
  ok1(MacCready::ArrivesAtBestGlide(polar, glide_settings, between));

  const GlideState low(vector, 0,
                       nav_between - 40, wind);
  ok1(!MacCready::ArrivesAtBestGlide(polar, glide_settings, low));
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

int main()
{
  plan_tests(2108);

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

  TestArrivesAtBestGlide();

  return exit_status();
}
