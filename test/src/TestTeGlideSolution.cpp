// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Tests for glide solutions when the aircraft "height" budget is expressed as
// total-energy altitude (nav altitude + kinetic height from TAS), matching
// MoreData::total_energy_height / Math/EnergyHeight.hpp / BasicComputer.
//
// The Engine layer only sees a scalar altitude in GlideState; these tests
// document the contract: equal TE inputs → equal MacCready solutions.

#include "Geo/Gravity.hpp"
#include "Geo/SpeedVector.hpp"
#include "Math/EnergyHeight.hpp"
#include "Math/Util.hpp"
#include "Engine/GlideSolvers/GlideSettings.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"

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

int
main()
{
  plan_tests(11);

  TestEquivalentTeSameSolution();
  TestTeNotBelowNavOnlyMargin();

  return exit_status();
}
