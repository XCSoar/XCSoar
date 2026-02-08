// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TestUtil.hpp"

#include <cmath>
#include <algorithm>

extern "C" {
#include "tap.h"
}

/**
 * DMSt distance scoring formula (Section 4.1.4):
 * score = distance_km * (1 + bonus) * 100 / index
 *
 * Where bonus is the sum of applicable bonus percentages:
 * - Triangle: +40% (Section 4.1.5.1)
 * - Out-and-return: +30% (Section 4.1.5.3)
 * - Quadrilateral: +40% (Section 4.1.5.2, declared only)
 * - Declared task: +30% (Section 4.1.5, not available in-flight)
 * - Multiple rounds: +20% (Section 4.1.5.4, declared only)
 *
 * Minimum score: 50 points (below = 0)
 *
 * @see https://github.com/weglide/dmst-wettbewerbsordnung
 */
static double
CalculateDMStDistanceScore(double distance_m, double bonus,
                           unsigned index) noexcept
{
  double distance_km = distance_m / 1000.0;
  return distance_km * (1.0 + bonus) * 100.0 / index;
}

/**
 * Apply DMSt minimum threshold for distance scoring.
 * Section 4: "Als Mindestwertung gilt fuer die Streckenwertung 50 Punkte"
 */
static double
ApplyDMStDistanceMinimum(double score) noexcept
{
  return score >= 50.0 ? score : 0.0;
}

/**
 * DMSt speed scoring formula (Section 4.2):
 * score = (speed_distance_km / 2) * 100 / ((index - 100) * 0.75 + 100)
 *
 * Minimum score: 25 points (below = 0)
 */
static double
CalculateDMStSpeedScore(double distance_m, unsigned index) noexcept
{
  double distance_km = distance_m / 1000.0;
  double effective_index = (static_cast<double>(index) - 100.0) * 0.75 + 100.0;
  return (distance_km / 2.0) * 100.0 / effective_index;
}

/**
 * Apply DMSt minimum threshold for speed scoring.
 * Section 4: "fuer die Geschwindigkeitswertung 25 Punkte"
 */
static double
ApplyDMStSpeedMinimum(double score) noexcept
{
  return score >= 25.0 ? score : 0.0;
}

static double
RoundScore(double score) noexcept
{
  return std::round(score * 100.0) / 100.0;
}

/**
 * Test basic distance scoring without bonuses.
 * Formula: distance_km * 100 / index
 */
static void
TestDistanceScoring()
{
  // 300 km, index 100, no bonus → 300 pts
  double score = CalculateDMStDistanceScore(300000, 0.0, 100);
  ok1(equals(score, 300.0));

  // 300 km, index 115, no bonus → 260.87 pts
  score = CalculateDMStDistanceScore(300000, 0.0, 115);
  double rounded = RoundScore(score);
  ok1(equals(rounded, 260.87));

  // 300 km, index 106, no bonus → 283.02 pts
  score = CalculateDMStDistanceScore(300000, 0.0, 106);
  rounded = RoundScore(score);
  ok1(equals(rounded, 283.02));

  // 500 km, index 100, no bonus → 500 pts
  score = CalculateDMStDistanceScore(500000, 0.0, 100);
  ok1(equals(score, 500.0));

  // 100 km, index 100, no bonus → 100 pts
  score = CalculateDMStDistanceScore(100000, 0.0, 100);
  ok1(equals(score, 100.0));
}

/**
 * Test triangle bonus scoring.
 * Section 4.1.5.1: +40% bonus for valid triangles.
 */
static void
TestTriangleBonus()
{
  // 300 km triangle, index 100 → 300 * 1.4 = 420 pts
  double score = CalculateDMStDistanceScore(300000, 0.40, 100);
  ok1(equals(score, 420.0));

  // 300 km triangle, index 115 → 300 * 1.4 * 100/115 = 365.22 pts
  score = CalculateDMStDistanceScore(300000, 0.40, 115);
  double rounded = RoundScore(score);
  ok1(equals(rounded, 365.22));

  // 500 km triangle, index 106 → 500 * 1.4 * 100/106 = 660.38 pts
  score = CalculateDMStDistanceScore(500000, 0.40, 106);
  rounded = RoundScore(score);
  ok1(equals(rounded, 660.38));
}

/**
 * Test out-and-return bonus scoring.
 * Section 4.1.5.3: +30% bonus for out-and-return.
 */
static void
TestORBonus()
{
  // 200 km OR, index 100 → 200 * 1.3 = 260 pts
  double score = CalculateDMStDistanceScore(200000, 0.30, 100);
  ok1(equals(score, 260.0));

  // 200 km OR, index 115 → 200 * 1.3 * 100/115 = 226.09 pts
  score = CalculateDMStDistanceScore(200000, 0.30, 115);
  double rounded = RoundScore(score);
  ok1(equals(rounded, 226.09));

  // 150 km OR, index 106 → 150 * 1.3 * 100/106 = 183.96 pts
  score = CalculateDMStDistanceScore(150000, 0.30, 106);
  rounded = RoundScore(score);
  ok1(equals(rounded, 183.96));
}

/**
 * Test the minimum threshold for distance scoring.
 * Section 4: minimum 50 points, below yields 0.
 */
static void
TestDistanceMinimum()
{
  // Score of 100 → passes threshold
  ok1(equals(ApplyDMStDistanceMinimum(100.0), 100.0));

  // Score of 50 → exactly at threshold
  ok1(equals(ApplyDMStDistanceMinimum(50.0), 50.0));

  // Score of 49.99 → below threshold
  ok1(equals(ApplyDMStDistanceMinimum(49.99), 0.0));

  // Score of 0 → below threshold
  ok1(equals(ApplyDMStDistanceMinimum(0.0), 0.0));

  // 30 km, index 100, no bonus → 30 pts → below threshold
  double score = CalculateDMStDistanceScore(30000, 0.0, 100);
  ok1(equals(ApplyDMStDistanceMinimum(score), 0.0));

  // 50 km, index 100, no bonus → 50 pts → at threshold
  score = CalculateDMStDistanceScore(50000, 0.0, 100);
  ok1(equals(ApplyDMStDistanceMinimum(score), 50.0));
}

/**
 * Test speed scoring formula.
 * Section 4.2: (distance_km / 2) * 100 / ((index - 100) * 0.75 + 100)
 */
static void
TestSpeedScoring()
{
  // 200 km, index 100 → (200/2) * 100 / 100 = 100 pts
  double score = CalculateDMStSpeedScore(200000, 100);
  ok1(equals(score, 100.0));

  // 200 km, index 120 → (200/2) * 100 / ((120-100)*0.75+100)
  //                     = 100 * 100 / 115 = 86.96 pts
  score = CalculateDMStSpeedScore(200000, 120);
  double rounded = RoundScore(score);
  ok1(equals(rounded, 86.96));

  // 300 km, index 106 → (300/2) * 100 / ((106-100)*0.75+100)
  //                     = 150 * 100 / 104.5 = 143.54 pts
  score = CalculateDMStSpeedScore(300000, 106);
  rounded = RoundScore(score);
  ok1(equals(rounded, 143.54));

  // Index 100 means effective_index = 100 (no correction)
  score = CalculateDMStSpeedScore(100000, 100);
  ok1(equals(score, 50.0));
}

/**
 * Test speed scoring minimum threshold.
 * Section 4: minimum 25 points for speed scoring.
 */
static void
TestSpeedMinimum()
{
  // Score of 30 → passes threshold
  ok1(equals(ApplyDMStSpeedMinimum(30.0), 30.0));

  // Score of 25 → exactly at threshold
  ok1(equals(ApplyDMStSpeedMinimum(25.0), 25.0));

  // Score of 24.99 → below threshold
  ok1(equals(ApplyDMStSpeedMinimum(24.99), 0.0));

  // 30 km in 2h, index 100 → (30/2) * 100 / 100 = 15 pts → below
  double score = CalculateDMStSpeedScore(30000, 100);
  ok1(equals(ApplyDMStSpeedMinimum(score), 0.0));
}

/**
 * Test composite scoring: the best of quad, triangle, OR
 * should be selected as the final distance score.
 */
static void
TestCompositeScoring()
{
  unsigned index = 106;

  // Quad: 300 km, no bonus → 283.02
  double quad_score = CalculateDMStDistanceScore(300000, 0.0, index);

  // Triangle: 250 km, +40% → 250 * 1.4 * 100/106 = 330.19
  double tri_score = CalculateDMStDistanceScore(250000, 0.40, index);

  // OR: 200 km, +30% → 200 * 1.3 * 100/106 = 245.28
  double or_score = CalculateDMStDistanceScore(200000, 0.30, index);

  // Triangle should win (330.19 > 283.02 > 245.28)
  double best = std::max({quad_score, tri_score, or_score});
  ok1(equals(RoundScore(best), RoundScore(tri_score)));

  // Now with a larger quadrilateral: 500 km, no bonus → 471.70
  quad_score = CalculateDMStDistanceScore(500000, 0.0, index);

  // Quadrilateral should now win (471.70 > 330.19 > 245.28)
  best = std::max({quad_score, tri_score, or_score});
  ok1(equals(RoundScore(best), RoundScore(quad_score)));
}

/**
 * Test triangle validation rules.
 * Section 4.1.5.1:
 * - Small triangle (<500km): shortest leg >= 28% of total
 * - Large triangle (>=500km): shortest >= 25%, longest <= 45%
 * These are the same as FAI/OLC triangle rules.
 */
static void
TestTriangleValidation()
{
  // Valid small equilateral triangle: 100km each leg, total 300km
  // Each leg is 33.3% → valid (>= 28%)
  {
    unsigned a = 100000, b = 100000, c = 100000;
    unsigned total = a + b + c;
    unsigned shortest = std::min({a, b, c});
    // Check: shortest * 25 >= total * 7 (28%)
    ok1(shortest * 25 >= total * 7);
  }

  // Invalid small triangle: one very short leg
  // 10km, 140km, 150km → total 300km, shortest=10km=3.3% → invalid
  {
    unsigned a = 10000, b = 140000, c = 150000;
    unsigned total = a + b + c;
    unsigned shortest = std::min({a, b, c});
    ok1(!(shortest * 25 >= total * 7));
  }

  // Valid small triangle at boundary: 28% exactly
  // 84km, 100km, 116km → total 300km, shortest=84km=28%
  {
    unsigned a = 84000, b = 100000, c = 116000;
    unsigned total = a + b + c;
    unsigned shortest = std::min({a, b, c});
    ok1(shortest * 25 >= total * 7);
  }

  // Valid large triangle (>= 500km): 25% shortest, 45% longest
  // 170km, 180km, 200km → total 550km
  // shortest=170km=30.9%, longest=200km=36.4% → valid
  {
    unsigned a = 170000, b = 180000, c = 200000;
    unsigned total = a + b + c;
    unsigned shortest = std::min({a, b, c});
    auto [s, longest] = std::minmax({a, b, c});
    ok1(shortest * 4 >= total && longest * 20 < total * 9);
  }

  // Invalid large triangle: longest leg too long
  // 130km, 130km, 260km → total 520km
  // shortest=130km=25%, longest=260km=50% → invalid (>45%)
  {
    unsigned a = 130000, b = 130000, c = 260000;
    unsigned total = a + b + c;
    unsigned shortest = std::min({a, b, c});
    auto [s, longest] = std::minmax({a, b, c});
    ok1(!(shortest * 4 >= total && longest * 20 < total * 9));
  }
}

/**
 * Test score rounding to two decimal places.
 */
static void
TestScoreRounding()
{
  ok1(equals(RoundScore(123.456789), 123.46));
  ok1(equals(RoundScore(123.454), 123.45));
  ok1(equals(RoundScore(123.455), 123.46));
  ok1(equals(RoundScore(100.12), 100.12));
}

int
main()
{
  plan_tests(5 + 3 + 3 + 6 + 4 + 4 + 2 + 5 + 4);

  TestDistanceScoring();
  TestTriangleBonus();
  TestORBonus();
  TestDistanceMinimum();
  TestSpeedScoring();
  TestSpeedMinimum();
  TestCompositeScoring();
  TestTriangleValidation();
  TestScoreRounding();

  return exit_status();
}
