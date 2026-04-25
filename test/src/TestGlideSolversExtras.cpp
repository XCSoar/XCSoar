// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Unit tests for GlideState / GlideResult helpers not covered by TestMacCready
// or TestGlidePolar (CalcAverageSpeed, DriftedDistance, InstantSpeed, Add,
// ground-angle helpers, CalcDeferred / cruise bearing).

#include "Engine/GlideSolvers/GlideSettings.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Navigation/Aircraft.hpp"
#include "Geo/SpeedVector.hpp"
#include "Math/Util.hpp"
#include "time/FloatDuration.hxx"

#include "TestUtil.hpp"

#include <cmath>

/** Expected DriftedDistance from GlideState.cpp geometry (must stay in sync). */
[[gnu::const]]
static double
ExpectedDriftedDistance(const GeoVector &vector, const SpeedVector &wind,
                        double time_s) noexcept
{
  if (wind.IsZero())
    return vector.distance;

  const double distance_wind = wind.norm * time_s;
  auto sc_wind = wind.bearing.Reciprocal().SinCos();
  const double sin_wind = sc_wind.first;
  const double cos_wind = sc_wind.second;

  const double distance_task = vector.distance;
  auto sc_task = vector.bearing.SinCos();
  const double sin_task = sc_task.first;
  const double cos_task = sc_task.second;

  const double dx = distance_task * sin_task - distance_wind * sin_wind;
  const double dy = distance_task * cos_task - distance_wind * cos_wind;
  return hypot(dx, dy);
}

static void
TestCalcAverageSpeedNoWind()
{
  const GlideState gs(GeoVector(5000., Angle::Zero()), 300., 1500.,
                      SpeedVector::Zero());
  ok1(equals(gs.CalcAverageSpeed(22.5), 22.5));
}

static void
TestCalcAverageSpeedWithWind()
{
  const GlideState gs(GeoVector(10000., Angle::Zero()), 500., 2500.,
                      SpeedVector(Angle::Zero(), 8.));
  const double v = 30.;
  const double vg = gs.CalcAverageSpeed(v);
  ok1(vg > 0.);
  ok1(!equals(vg, v));
}

static void
TestDriftedDistance()
{
  const GeoVector vec(7500., Angle::Degrees(40.));
  const GlideState gs_no_wind(vec, 0., 1000., SpeedVector::Zero());
  ok1(equals(gs_no_wind.DriftedDistance(FloatDuration{60.}), vec.distance));

  const SpeedVector wind(Angle::Degrees(115.), 12.);
  const GlideState gs_wind(vec, 0., 1000., wind);
  const double t = 90.;
  ok1(equals(gs_wind.DriftedDistance(FloatDuration{t}),
             ExpectedDriftedDistance(vec, wind, t)));
}

static void
TestGlideResultGroundAnglesAndFinalGlide()
{
  GlideResult r{};
  r.validity = GlideResult::Validity::OK;
  r.vector = GeoVector(0., Angle::Zero());
  r.pure_glide_height = 100.;
  r.altitude_difference = 50.;
  ok1(equals(r.GlideAngleGround(), 1000.));
  ok1(equals(r.DestinationAngleGround(), 1000.));

  r.vector = GeoVector(2000., Angle::Zero());
  ok1(equals(r.GlideAngleGround(), 100. / 2000.));
  ok1(equals(r.DestinationAngleGround(), (50. + 100.) / 2000.));

  r.height_climb = 0.;
  ok1(r.IsFinalGlide());

  r.height_climb = 1.;
  ok1(!r.IsFinalGlide());
}

static void
TestGlideResultAddDistances()
{
  GlideSettings settings;
  settings.SetDefaults();
  GlidePolar polar(0.);

  const SpeedVector wind = SpeedVector::Zero();
  GlideState gs1(GeoVector(4000., Angle::Zero()), 500., 2200., wind);
  GlideState gs2(GeoVector(2500., Angle::Zero()), 500., 2200., wind);

  GlideResult r1 = MacCready::Solve(settings, polar, gs1);
  GlideResult r2 = MacCready::Solve(settings, polar, gs2);
  ok1(r1.IsOk());
  ok1(r2.IsOk());

  const double d1 = r1.vector.distance;
  const double d2 = r2.vector.distance;
  r1.Add(r2);
  ok1(equals(r1.vector.distance, d1 + d2));
}

static void
TestGlideResultInstantSpeedMcZero()
{
  GlideResult leg;
  leg.vector = GeoVector(10000., Angle::Zero());
  leg.head_wind = 0.;

  AircraftState ac{};
  ac.ground_speed = 40.;
  ac.track = Angle::Zero();

  GlidePolar polar(0.);
  polar.SetMC(0.);

  const double v_a =
    (leg.vector.bearing - ac.track).cos() * ac.ground_speed;
  ok1(equals(leg.InstantSpeed(ac, leg, polar), v_a));
}

static void
TestGlideResultInstantSpeedMcPositive()
{
  GlideResult leg;
  leg.vector = GeoVector(8000., Angle::Zero());
  leg.head_wind = 2.;

  AircraftState ac{};
  ac.ground_speed = 35.;
  ac.track = Angle::Zero();
  ac.vario = 1.2;

  GlidePolar polar(0.);
  polar.SetMC(2.);
  ok1(polar.GetMC() > 0.);

  const double inst = leg.InstantSpeed(ac, leg, polar);
  ok1(std::isfinite(inst));
  ok1(inst > 0.);
}

static void
TestCalcDeferredCruiseBearing()
{
  GlideSettings settings;
  settings.SetDefaults();
  GlidePolar polar(0.);
  const SpeedVector wind = SpeedVector::Zero();

  GlideState gs(GeoVector(12000., Angle::Degrees(25.)), 400., 2000., wind);
  GlideResult r = MacCready::Solve(settings, polar, gs);
  ok1(r.IsOk());

  r.CalcDeferred();
  ok1(equals(r.cruise_track_bearing, r.vector.bearing));
}

static void
TestCalcDeferredCruiseBearingWindAlongTrack()
{
  /* Wind aligned with task: effective_wind_angle.sin() == 0 → no asin
     correction; cruise matches task bearing. */
  GlideSettings settings;
  settings.SetDefaults();
  GlidePolar polar(0.);
  const GeoVector vec(9000., Angle::Degrees(0.));
  const SpeedVector w(vec.bearing, 6.);

  GlideState gs(vec, 500., 1800., w);
  GlideResult r = MacCready::Solve(settings, polar, gs);
  ok1(r.IsOk());
  ok1(r.effective_wind_speed > 0.);

  r.CalcDeferred();
  ok1(equals(r.cruise_track_bearing, r.vector.bearing));
}

int
main()
{
  plan_tests(23);

  TestCalcAverageSpeedNoWind();
  TestCalcAverageSpeedWithWind();
  TestDriftedDistance();
  TestGlideResultGroundAnglesAndFinalGlide();
  TestGlideResultAddDistances();
  TestGlideResultInstantSpeedMcZero();
  TestGlideResultInstantSpeedMcPositive();
  TestCalcDeferredCruiseBearing();
  TestCalcDeferredCruiseBearingWindAlongTrack();

  return exit_status();
}
