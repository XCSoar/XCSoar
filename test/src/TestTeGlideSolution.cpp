// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Tests for glide solutions when the aircraft "height" budget uses convertible
// energy (nav altitude + (v² − v_bg²)/(2g)), matching the XCSoar manual and
// Math/EnergyHeight.hpp / GlideEnergyHeight / BasicComputer.
//
// Covers: MacCready convertible-energy contract; Math/EnergyHeight;
// GlideHeightForMacCready; GlideEnergyHeight; ToAircraftState wiring;
// AircraftStateFilter prediction keeping (TE − nav) invariant when both
// heights get the same vario offset.

#include "NMEA/Aircraft.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "Computer/BasicComputer.hpp"
#include "Computer/Settings.hpp"
#include "Atmosphere/Pressure.hpp"
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

/** Two different (nav, TAS) pairs that yield the same convertible energy. */
static void
EquivalentConvertiblePair(double v_bg, double &nav1, double &tas1,
                          double &nav2, double &tas2) noexcept
{
  nav1 = 2500.;
  tas1 = 30.;
  nav2 = 2400.;
  const double te = GlideConvertibleEnergyHeight(nav1, tas1, v_bg);
  const double conv2 = te - nav2;
  tas2 = std::sqrt(std::max(0., 2. * GRAVITY * conv2 + Square(v_bg)));
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
 * Same convertible energy height, built from different nav/TAS combinations,
 * must produce the same glide solution for a fixed leg and polar.
 */
static void
TestEquivalentConvertibleSameSolution()
{
  GlideSettings settings;
  settings.SetDefaults();
  GlidePolar polar(0);
  const double v_bg = polar.GetVBestLD();

  double nav1, tas1, nav2, tas2;
  EquivalentConvertiblePair(v_bg, nav1, tas1, nav2, tas2);

  const double te1 = GlideConvertibleEnergyHeight(nav1, tas1, v_bg);
  const double te2 = GlideConvertibleEnergyHeight(nav2, tas2, v_bg);
  ok1(equals(te1, te2));

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
 * With MC = 0, more convertible energy (same nav, higher TAS above v_bg) must
 * not yield a worse glide margin than using nav altitude alone.
 */
static void
TestConvertibleNotBelowNavOnlyMargin()
{
  GlideSettings settings;
  settings.SetDefaults();
  GlidePolar polar(0);
  polar.SetMC(0);
  const double v_bg = polar.GetVBestLD();

  const GeoVector vector(8000., Angle::Zero());
  const SpeedVector wind = SpeedVector::Zero();
  constexpr double min_arrival = 300.;
  constexpr double nav = 1800.;
  const double tas = v_bg + 10.;

  const double te = GlideConvertibleEnergyHeight(nav, tas, v_bg);
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

  constexpr double v_bg = 20.;
  ok1(equals(ConvertibleKineticHeight(v_bg, v_bg), 0.));
  ok1(equals(GlideConvertibleEnergyHeight(nav, v_bg, v_bg), nav));
  ok1(equals(ConvertibleKineticHeight(tas, v_bg),
             KineticHeight(tas) - KineticHeight(v_bg)));
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
  ok1(equals(GlideEnergyHeight(m), 500.));

  m.true_airspeed = 25.;
  m.airspeed_available.Update(m.clock);
  constexpr double v_bg = 20.;
  const double expected =
    GlideConvertibleEnergyHeight(m.nav_altitude, m.true_airspeed, v_bg);
  ok1(equals(GlideEnergyHeight(m, v_bg), expected));
}

static void
TestToAircraftStateAndGlideHeight()
{
  DerivedInfo calculated{};
  calculated.wind_available.Clear();
  calculated.terrain_valid = false;
  GlidePolar polar(0);
  calculated.glide_polar_safety = polar;

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

  const double v_bg = polar.GetVBestLD();
  const AircraftState ac = ToAircraftState(info, calculated);
  ok1(equals(ac.altitude, info.nav_altitude));
  ok1(equals(ac.total_energy_height, GlideEnergyHeight(info, v_bg)));
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

/** NMEA-like fix with device true airspeed (airspeed_real). */
static void
SetupNmeaFix(MoreData &basic, TimeStamp clock, GeoPoint location,
             double nav_msl, double true_airspeed_m_s) noexcept
{
  basic.Reset();
  basic.clock = clock;
  basic.time = clock;
  basic.time_available.Update(clock);
  basic.location = location;
  basic.location_available.Update(clock);
  basic.gps_altitude = nav_msl;
  basic.gps_altitude_available.Update(clock);
  basic.true_airspeed = true_airspeed_m_s;
  basic.airspeed_real = true;
  basic.airspeed_available.Update(clock);
}

static void
RunBasicComputer(MoreData &basic, BasicComputer &computer,
                 MoreData &last, DerivedInfo &calculated,
                 const ComputerSettings &settings) noexcept
{
  FeaturesSettings features{};
  features.nav_baro_altitude_enabled = false;
  computer.Fill(basic, AtmosphericPressure::Standard(), features);
  computer.Compute(basic, last, last, calculated, settings);
  last = basic;
}

/**
 * BasicComputer still stores full TE in MoreData; glide uses convertible energy
 * via GlideEnergyHeight / ToAircraftState after the merge path.
 */
static void
TestBasicComputerConvertibleGlidePipeline()
{
  GlidePolar polar(0);
  polar.SetMC(0);
  const double v_bg = polar.GetVBestLD();

  DerivedInfo calculated{};
  calculated.glide_polar_safety = polar;
  calculated.wind_available.Clear();
  calculated.terrain_valid = false;

  ComputerSettings settings{};
  settings.polar.glide_polar_task = polar;

  BasicComputer computer;
  MoreData last{};
  last.Reset();

  const GeoPoint location(Angle::Degrees(8.5), Angle::Degrees(47.2));
  const TimeStamp t0{FloatDuration{1000.}};
  const TimeStamp t1{FloatDuration{1001.}};

  double nav1, tas1, nav2, tas2;
  EquivalentConvertiblePair(v_bg, nav1, tas1, nav2, tas2);

  MoreData basic1;
  SetupNmeaFix(basic1, t0, location, nav1, tas1);
  RunBasicComputer(basic1, computer, last, calculated, settings);

  MoreData basic2;
  SetupNmeaFix(basic2, t1, location, nav2, tas2);
  RunBasicComputer(basic2, computer, last, calculated, settings);

  ok1(equals(basic1.energy_height, KineticHeight(tas1)));
  ok1(equals(basic1.total_energy_height, nav1 + KineticHeight(tas1)));
  ok1(equals(basic2.energy_height, KineticHeight(tas2)));
  ok1(equals(basic2.total_energy_height, nav2 + KineticHeight(tas2)));
  ok1(equals(basic1.total_energy_height, basic2.total_energy_height));

  const double glide1 = GlideEnergyHeight(basic1, v_bg);
  const double glide2 = GlideEnergyHeight(basic2, v_bg);
  ok1(equals(glide1, glide2));
  ok1(!equals(glide1, basic1.total_energy_height));

  const AircraftState ac1 = ToAircraftState(basic1, calculated);
  const AircraftState ac2 = ToAircraftState(basic2, calculated);
  ok1(equals(ac1.total_energy_height, glide1));
  ok1(equals(ac2.total_energy_height, glide2));

  GlideSettings glide_settings;
  glide_settings.SetDefaults();
  const GeoVector vector(12000., Angle::Zero());
  const SpeedVector wind = SpeedVector::Zero();
  constexpr double min_arrival = 400.;

  const GlideState gs1(vector, min_arrival, GlideHeightForMacCready(ac1), wind);
  const GlideState gs2(vector, min_arrival, GlideHeightForMacCready(ac2), wind);

  const GlideResult r1 = MacCready::Solve(glide_settings, polar, gs1);
  const GlideResult r2 = MacCready::Solve(glide_settings, polar, gs2);
  AssertSameGlideResult(r1, r2);
}

int
main()
{
  plan_tests(48);

  TestEquivalentConvertibleSameSolution();
  TestConvertibleNotBelowNavOnlyMargin();
  TestEnergyHeightMath();
  TestGlideHeightForMacCready();
  TestGlideEnergyHeight();
  TestToAircraftStateAndGlideHeight();
  TestAircraftStateFilterKineticGapInPrediction();
  TestBasicComputerConvertibleGlidePipeline();

  return exit_status();
}
