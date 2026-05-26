// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StringAPI.hxx"
#include "TestUtil.hpp"

#include <chrono>
#include <stdexcept>

using namespace std::chrono;

static bool
ThrowsParse(const char *iso)
{
  try {
    (void)ParseISO8601Utc(iso);
    return false;
  } catch (const std::exception &) {
    return true;
  }
}

int
main()
{
  plan_tests(12);

  const auto tp = ParseISO8601Utc("2026-04-09T08:00:00Z");
  const BrokenDateTime dt(tp);
  ok1(dt.year == 2026 && dt.month == 4 && dt.day == 9);
  ok1(dt.hour == 8 && dt.minute == 0 && dt.second == 0);

  ok1(!ThrowsParse("2026-04-09T08:00:00.789Z"));
  ok1(!ThrowsParse("2026-04-09T08:00:00.123Z"));

  ok1(ThrowsParse("2026-02-31T08:00:00Z"));
  ok1(ThrowsParse("2026-04-09T08:00:00"));
  ok1(ThrowsParse("2026-04-09T08:00:00+02:00"));
  ok1(ThrowsParse("PERM"));
  ok1(ThrowsParse("not-a-date"));

  char buffer[32];
  FormatISO8601(buffer, dt);
  ok1(StringIsEqual(buffer, "2026-04-09T08:00:00Z"));

  const auto round_trip = ParseISO8601Utc(buffer);
  ok1(round_trip == tp);

  const auto fractional = ParseISO8601Utc("2026-04-09T08:00:00.789Z");
  const BrokenDateTime fractional_dt(fractional);
  ok1(fractional_dt.hour == 8 && fractional_dt.minute == 0 &&
      fractional_dt.second == 0);

  return exit_status();
}
