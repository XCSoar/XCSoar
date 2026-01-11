// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TestUtil.hpp"

#include <cmath>
#include <algorithm>

extern "C" {
#include "tap.h"
}

static double
CalculateWeglideDistanceScore(double distance_m, unsigned handicap) noexcept
{
  // 1.0 raw points per km, then ApplyHandicap: (raw × 100) / handicap
  double raw_points = distance_m / 1000.0 * 1.0;
  return (raw_points * 100.0) / handicap;
}

static double
CalculateWeglideFAIScore(double distance_m, unsigned handicap) noexcept
{
  // 0.3 raw points per km, then ApplyHandicap: (raw × 100) / handicap
  double raw_points = distance_m / 1000.0 * 0.3;
  return (raw_points * 100.0) / handicap;
}

static double
CalculateWeglideORScore(double distance_m, unsigned handicap) noexcept
{
  // 0.2 raw points per km, then ApplyHandicap: (raw × 100) / handicap
  double raw_points = distance_m / 1000.0 * 0.2;
  return (raw_points * 100.0) / handicap;
}

/**
 * Calculate expected WeGlide Free score according to official rules.
 * Formula: (free_raw_points × 100) / index
 * Where free_raw = distance_raw + max(fai_raw, or_raw)
 * Raw points per km: Distance=1.0, FAI=0.3, OR=0.2
 */
static double
CalculateWeglideFreeScore(double distance_m, double fai_m, double or_m,
                          unsigned handicap) noexcept
{
  // Raw points per km: Distance=1.0, FAI=0.3, OR=0.2
  double distance_raw = distance_m / 1000.0 * 1.0;
  double fai_raw = fai_m / 1000.0 * 0.3;
  double or_raw = or_m / 1000.0 * 0.2;

  double area_bonus = std::max(fai_raw, or_raw);
  double free_raw = distance_raw + area_bonus;

  double score = (free_raw * 100.0) / handicap;

  if (score < 50.0)
    score = 0.0;

  score = std::round(score * 100.0) / 100.0;

  return score;
}

static double
RoundScore(double score) noexcept
{
  return std::round(score * 100.0) / 100.0;
}

static double
ApplyMinimumScore(double score) noexcept
{
  if (score < 50.0)
    return 0.0;
  return score;
}

static void
TestWeglideDistance()
{
  double expected = CalculateWeglideDistanceScore(149000, 107);
  double rounded = RoundScore(expected);
  ok1(equals(rounded, 139.25));

  expected = CalculateWeglideDistanceScore(100000, 100);
  ok1(equals(expected, 100.0));

  expected = CalculateWeglideDistanceScore(200000, 120);
  rounded = RoundScore(expected);
  ok1(equals(rounded, 166.67));

  expected = CalculateWeglideDistanceScore(50000, 107);
  rounded = RoundScore(expected);
  ok1(equals(rounded, 46.73));
}

static void
TestWeglideFAI()
{
  double expected = CalculateWeglideFAIScore(300000, 107);
  double rounded = RoundScore(expected);
  ok1(equals(rounded, 84.11));

  expected = CalculateWeglideFAIScore(100000, 100);
  ok1(equals(expected, 30.0));

  expected = CalculateWeglideFAIScore(200000, 120);
  ok1(equals(expected, 50.0));
}

static void
TestWeglideOR()
{
  double expected = CalculateWeglideORScore(200000, 107);
  double rounded = RoundScore(expected);
  ok1(equals(rounded, 37.38));

  expected = CalculateWeglideORScore(100000, 100);
  ok1(equals(expected, 20.0));

  expected = CalculateWeglideORScore(150000, 120);
  ok1(equals(expected, 25.0));
}

static void
TestWeglideFree()
{
  double expected = CalculateWeglideFreeScore(149000, 100000, 80000, 107);
  double rounded = RoundScore(expected);
  ok1(equals(rounded, 167.29));

  expected = CalculateWeglideFreeScore(100000, 100000, 166670, 100);
  rounded = RoundScore(expected);
  ok1(equals(rounded, 133.33));

  expected = CalculateWeglideFreeScore(100000, 166670, 100000, 100);
  rounded = RoundScore(expected);
  ok1(equals(expected, 150.0));

  expected = CalculateWeglideFreeScore(100000, 200000, 200000, 100);
  rounded = RoundScore(expected);
  ok1(equals(expected, 160.0));
}

static void
TestMinimumScore()
{
  double score = (5.0 * 100.0) / 107.0;
  double result = ApplyMinimumScore(score);
  ok1(equals(result, 0.0));

  score = (5.35 * 100.0) / 107.0;
  result = ApplyMinimumScore(score);
  ok1(equals(result, 0.0));

  score = (53.5 * 100.0) / 107.0;
  result = ApplyMinimumScore(score);
  ok1(equals(result, 50.0));

  score = (53.6 * 100.0) / 107.0;
  result = ApplyMinimumScore(score);
  double rounded = RoundScore(result);
  ok1(equals(rounded, 50.09));
}

static void
TestScoreRounding()
{
  double score = 123.456789;
  double rounded = RoundScore(score);
  ok1(equals(rounded, 123.46));

  score = 123.454;
  rounded = RoundScore(score);
  ok1(equals(rounded, 123.45));

  score = 123.455;
  rounded = RoundScore(score);
  ok1(equals(rounded, 123.46));

  score = 123.456;
  rounded = RoundScore(score);
  ok1(equals(rounded, 123.46));

  score = 100.12;
  rounded = RoundScore(score);
  ok1(equals(rounded, 100.12));
}

int
main()
{
  plan_tests(4 + 3 + 3 + 4 + 4 + 5);

  TestWeglideDistance();
  TestWeglideFAI();
  TestWeglideOR();
  TestWeglideFree();
  TestMinimumScore();
  TestScoreRounding();

  return exit_status();
}

