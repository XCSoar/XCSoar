// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Tests for glide solutions when the aircraft "height" budget is expressed as
// total-energy altitude (nav altitude + kinetic height from TAS), matching
// MoreData::total_energy_height / Math/EnergyHeight.hpp / BasicComputer.
//
// Covers: MacCready TE contract; Math/EnergyHeight; GlideHeightForMacCready;
// GlideEnergyHeight; ToAircraftState wiring; AircraftStateFilter prediction
// keeping (TE − nav) invariant when both heights get the same vario offset.

#include "NMEA/Aircraft.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "Engine/GlideSolvers/GlideSettings.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Engine/Util/AircraftStateFilter.hpp"
#include "Geo/Gravity.hpp"
#include "Geo/SpeedVector.hpp"
#include "Math/EnergyHeight.hpp"
#include "Math/Util.hpp"
#include "Navigation/Aircraft.hpp"
#include "time/FloatDuration.hxx"
#include "time/Stamp.hpp"

#include "TestUtil.hpp"

#include <cmath>

/** Two different (nav, TAS) pairs that yield the same total energy height. */
static void
EquivalentTePair(double &nav1, double &tas1, double &nav2, double &tas2) noexcept
{
  nav1 = 2500.;
  tas1 = 30.;
  nav2 = 2400.;
  const double te = TotalEnergyHeight(nav1, tas1);
  const double e2 = te - nav2;
  tas2 = std::sqrt(std::max(0., 2. * GRAVITY * e2));
}

static void
AssertSameGlideResult(const GlideResult &a, const GlideResult &b)
{
  ok1(a.validity == b.validity);
  ok1(equals(a.altitude_difference, b.altitude_difference));
  ok1(equals(a.height_glide, b.height_glide));
  ok1(equals(a.height_climb, b.height_climb));
  ok1(equals(a.vector.distance, b.vector.distance));
  ok1(equals(a.v_opt, b.v_opt));
}

/**
 * Same total-energy height, built from different nav/TAS combinations, must
 * produce the same glide solution for a fixed leg and polar.
 */
static void
TestEquivalentTeSameSolution()
{
  double nav1, tas1, nav2, tas2;
  EquivalentTePair(nav1, tas1, nav2, tas2);

  const double te1 = TotalEnergyHeight(nav1, tas1);
  const double te2 = TotalEnergyHeight(nav2, tas2);
  ok1(equals(te1, te2));

  GlideSettings settings;
  settings.SetDefaults();
  GlidePolar polar(0);

  const GeoVector vector(10000., Angle::Zero());
  const SpeedVector wind = SpeedVector::Zero();
  constexpr double min_arrival = 500.;

  const GlideState gs1(vector, min_arrival, te1, wind);
  const GlideState gs2(vector, min_arrival, te2, wind);

  const GlideResult r1 = MacCready::Solve(settings, polar, gs1);
  const GlideResult r2 = MacCready::Solve(settings, polar, gs2);

  AssertSameGlideResult(r1, r2);
}

/**
 * With MC = 0, more energy height (same nav, higher TAS → higher TE) must not
 * yield a worse glide margin than using nav altitude alone.
 */
static void
TestTeNotBelowNavOnlyMargin()
{
  GlideSettings settings;
  settings.SetDefaults();
  GlidePolar polar(0);
  polar.SetMC(0);

  const GeoVector vector(8000., Angle::Zero());
  const SpeedVector wind = SpeedVector::Zero();
  constexpr double min_arrival = 300.;
  constexpr double nav = 1800.;
  constexpr double tas = 35.;

  const double te = TotalEnergyHeight(nav, tas);
  ok1(te > nav);

  const GlideState gs_nav(vector, min_arrival, nav, wind);
  const GlideState gs_te(vector, min_arrival, te, wind);

  const GlideResult r_nav = MacCready::Solve(settings, polar, gs_nav);
  const GlideResult r_te = MacCready::Solve(settings, polar, gs_te);

  ok1(r_nav.IsOk());
  ok1(r_te.IsOk());
  ok1(r_te.altitude_difference >= r_nav.altitude_difference);
}

static void
TestEnergyHeightMath()
{
  ok1(equals(2. * GRAVITY * Inverse2G(), 1.));
  ok1(equals(KineticHeight(0.), 0.));
  ok1(equals(TotalEnergyHeight(1200., 0.), 1200.));

  constexpr double tas = 22.5;
  const double ke = KineticHeight(tas);
  const double tas_back = std::sqrt(std::max(0., 2. * GRAVITY * ke));
  ok1(equals(tas, tas_back));

  const double nav = 900.;
  ok1(equals(TotalEnergyHeight(nav, tas), nav + KineticHeight(tas)));
}

static void
TestGlideHeightForMacCready()
{
  AircraftState ac{};
  ac.altitude = 900.;
  ac.total_energy_height = 0.;
  ok1(equals(GlideHeightForMacCready(ac), 900.));

  ac.total_energy_height = 100.;
  ok1(equals(GlideHeightForMacCready(ac), 100.));

  ac.total_energy_height = -1.;
  ok1(equals(GlideHeightForMacCready(ac), ac.altitude));
}

static void
TestGlideEnergyHeight()
{
  MoreData m;
  m.Reset();
  m.total_energy_height = 9999.;
  ok1(equals(GlideEnergyHeight(m), 0.));

  m.clock = TimeStamp{FloatDuration{1.}};
  m.gps_altitude_available.Update(m.clock);
  m.nav_altitude = 500.;
  m.total_energy_height = 530.;
  ok1(equals(GlideEnergyHeight(m), 530.));
}

static void
TestToAircraftStateAndGlideHeight()
{
  DerivedInfo calculated{};
  calculated.wind_available.Clear();
  calculated.terrain_valid = false;

  MoreData info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{3600.}};
  info.gps_altitude = 1500.;
  info.nav_altitude = 1500.;
  info.gps_altitude_available.Update(info.clock);
  constexpr double tas = 25.;
  info.true_airspeed = tas;
  info.airspeed_available.Update(info.clock);
  info.energy_height = KineticHeight(tas);
  info.total_energy_height = info.nav_altitude + info.energy_height;

  const AircraftState ac = ToAircraftState(info, calculated);
  ok1(equals(ac.altitude, info.nav_altitude));
  ok1(equals(ac.total_energy_height, GlideEnergyHeight(info)));
  ok1(equals(GlideHeightForMacCready(ac), ac.total_energy_height));

  MoreData no_nav = info;
  no_nav.gps_altitude_available.Clear();
  no_nav.baro_altitude_available.Clear();
  ok1(equals(GlideEnergyHeight(no_nav), 0.));

  const AircraftState ac2 = ToAircraftState(no_nav, calculated);
  ok1(equals(ac2.altitude, 0.));
  ok1(equals(ac2.total_energy_height, 0.));
  ok1(equals(GlideHeightForMacCready(ac2), 0.));
}

static void
TestAircraftStateFilterKineticGapInPrediction()
{
  AircraftStateFilter f(30.);

  AircraftState s0{};
  s0.time = TimeStamp{FloatDuration{100.}};
  s0.location = GeoPoint(Angle::Degrees(8.5), Angle::Degrees(47.2));
  s0.altitude = 1000.;
  s0.total_energy_height = 1045.;

  AircraftState s1 = s0;
  s1.time = TimeStamp{FloatDuration{101.}};
  s1.altitude = 1003.;
  s1.total_energy_height = 1048.;

  f.Reset(s0);
  f.Update(s1);

  const AircraftState pred = f.GetPredictedState(FloatDuration{2.});
  const double gap_last = s1.total_energy_height - s1.altitude;
  const double gap_pred = pred.total_energy_height - pred.altitude;
  ok1(equals(gap_last, gap_pred));
}

int
main()
{
  plan_tests(29);

  TestEquivalentTeSameSolution();
  TestTeNotBelowNavOnlyMargin();
  TestEnergyHeightMath();
  TestGlideHeightForMacCready();
  TestGlideEnergyHeight();
  TestToAircraftStateAndGlideHeight();
  TestAircraftStateFilterKineticGapInPrediction();

  return exit_status();
}
