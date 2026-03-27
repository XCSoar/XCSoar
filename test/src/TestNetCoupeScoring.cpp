// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TestUtil.hpp"

#include <cmath>

extern "C" {
#include "tap.h"
}

/**
 * FFVP Coupe Fédérale (NetCoupe) scoring per WeGlide documentation:
 * https://docs.weglide.org/contests/national/ffvp_coupe_federale.html
 *
 * Points_Flight = Distance_km x (100 / Handicap_Glider) x Success_Index
 *
 * - "Free" flight: Success_Index = 1.0
 * - "As declared" (tasked): Success_Index = 1.2
 *
 * Matches NetCoupe::CalculateResult() for Success_Index = 1.0: raw
 * points per km = 1.0, then ApplyHandicap (×100/handicap), same pattern
 * as WeglideDistance.
 */
static double
CalculateFfvpScore(double distance_m, unsigned handicap,
                   double success_index) noexcept
{
  double distance_km = distance_m / 1000.0;
  return distance_km * (100.0 / handicap) * success_index;
}

static double
RoundScore(double score) noexcept
{
  return std::round(score * 100.0) / 100.0;
}

/**
 * Free-flight cases (Success_Index = 1.0); same numeric checks as
 * TestWeglideDistance (identical distance handicap scaling).
 */
static void
TestFfvpFreeFlight()
{
  double expected = CalculateFfvpScore(149000, 107, 1.0);
  double rounded = RoundScore(expected);
  ok1(equals(rounded, 139.25));

  expected = CalculateFfvpScore(100000, 100, 1.0);
  ok1(equals(expected, 100.0));

  expected = CalculateFfvpScore(200000, 120, 1.0);
  rounded = RoundScore(expected);
  ok1(equals(rounded, 166.67));

  expected = CalculateFfvpScore(50000, 107, 1.0);
  rounded = RoundScore(expected);
  ok1(equals(rounded, 46.73));
}

/**
 * Declared-flight coefficient from the same doc (electronic declaration
 * before takeoff).  Not applied in XCSoar NetCoupe::CalculateResult().
 */
static void
TestFfvpDeclaredSuccessIndex()
{
  double expected = CalculateFfvpScore(100000, 100, 1.2);
  ok1(equals(expected, 120.0));

  expected = CalculateFfvpScore(200000, 120, 1.2);
  ok1(equals(expected, 200.0));

  expected = CalculateFfvpScore(149000, 107, 1.2);
  double rounded = RoundScore(expected);
  ok1(equals(rounded, 167.10));
}

int
main()
{
  plan_tests(4 + 3);

  TestFfvpFreeFlight();
  TestFfvpDeclaredSuccessIndex();

  return exit_status();
}
