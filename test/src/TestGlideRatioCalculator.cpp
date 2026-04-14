// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/GlideRatioCalculator.hpp"
#include "Computer/Settings.hpp"

#include "TestUtil.hpp"

/**
 * Two samples in a partial buffer: nav height loss and TE height loss differ.
 * Verifies Calculate(true) uses total energy height and Calculate(false) nav.
 */
static void
TestTeVsNavHeightLoss()
{
  GlideRatioCalculator grc;
  ComputerSettings settings{};
  settings.average_eff_time = ae30seconds;
  grc.Initialize(settings);

  grc.Add(100, 1000, 1150);
  grc.Add(100, 900, 1000);

  const double dist = 200.;
  const double nav_diff = 1000 - 900;
  const double te_diff = 1150 - 1000;

  ok1(equals(grc.Calculate(false), dist / nav_diff));
  ok1(equals(grc.Calculate(true), dist / te_diff));
}

/**
 * If TE is unavailable (zero), TE mode falls back to the same nav-based loss as
 * non-TE mode.
 */
static void
TestTeFallbackWhenTeMissing()
{
  GlideRatioCalculator grc;
  ComputerSettings settings{};
  settings.average_eff_time = ae30seconds;
  grc.Initialize(settings);

  grc.Add(100, 1000, 0);
  grc.Add(100, 900, 0);

  ok1(equals(grc.Calculate(true), grc.Calculate(false)));
}

int
main()
{
  plan_tests(3);

  TestTeVsNavHeightLoss();
  TestTeFallbackWhenTeMissing();

  return exit_status();
}
