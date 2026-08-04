// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/SkySight/RegionTime.hpp"
#include "TestUtil.hpp"

#include <ctime>

static void
TestWarsawSummerOffset()
{
  /* 2026-08-05 13:30:00 UTC → CEST (UTC+2) */
  constexpr time_t utc = 1785936600;
  const auto fallback = RoughTimeDelta::FromSeconds(3600);
  const auto offset = SkySight::GetRegionUtcOffset("Europe/Warsaw", utc,
                                                   fallback);
  ok1(offset.AsSeconds() == 7200);

  const auto empty = SkySight::GetRegionUtcOffset("", utc, fallback);
  ok1(empty.AsSeconds() == 3600);
}

int
main()
{
  plan_tests(2);
  TestWarsawSummerOffset();
  return exit_status();
}
