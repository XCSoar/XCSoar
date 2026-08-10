// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/SkySight/RegionTime.hpp"
#include "TestUtil.hpp"

#include <ctime>

static void
TestWarsawSummerOffset()
{
  /* 2026-08-05 13:30:00 UTC → CEST (UTC+2) via region tz, not pilot offset */
  constexpr time_t utc = 1785936600;
  const auto offset = SkySight::GetRegionUtcOffset("Europe/Warsaw", utc);
  ok1(offset.AsSeconds() == 7200);

  const auto empty = SkySight::GetRegionUtcOffset("", utc);
  ok1(empty.AsSeconds() == 0);
}

int
main()
{
  plan_tests(2);
  TestWarsawSummerOffset();
  return exit_status();
}
