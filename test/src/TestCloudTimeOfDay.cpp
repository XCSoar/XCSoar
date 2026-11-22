// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Cloud/TimeOfDay.hpp"
#include "TestUtil.hpp"

using std::chrono::milliseconds;

static constexpr uint32_t HOUR = 60 * 60 * 1000;
static constexpr uint32_t DAY = 24 * HOUR;

int main()
{
  plan_tests(14);

  ok1(TimeOfDayDelta(10 * HOUR, 10 * HOUR) == milliseconds{0});
  ok1(TimeOfDayDelta(10 * HOUR + 5000, 10 * HOUR) == milliseconds{5000});
  ok1(TimeOfDayDelta(10 * HOUR, 10 * HOUR + 5000) == milliseconds{-5000});

  /* the server's clock wraps at midnight */
  ok1(TimeOfDayDelta(1000, DAY - 1000) == milliseconds{2000});
  ok1(TimeOfDayDelta(DAY - 1000, 1000) == milliseconds{-2000});

  /* a client's time keeps counting after midnight */
  ok1(TimeOfDayDelta(DAY + 1000, 1000) == milliseconds{0});
  ok1(TimeOfDayDelta(DAY + 1000, DAY - 1000) == milliseconds{2000});

  /* 12 hours apart: positive */
  ok1(TimeOfDayDelta(12 * HOUR, 0) == milliseconds{12 * HOUR});

  /* a fix up to one minute older is late */
  ok1(!IsLateFix(10 * HOUR, 10 * HOUR));
  ok1(!IsLateFix(10 * HOUR + 5000, 10 * HOUR));
  ok1(IsLateFix(10 * HOUR, 10 * HOUR + 5000));
  ok1(IsLateFix(10 * HOUR, 10 * HOUR + 60000));

  /* a bigger step back is a time warp, not a late fix */
  ok1(!IsLateFix(9 * HOUR, 10 * HOUR));

  /* one second before a fix at exactly midnight is late */
  ok1(IsLateFix(DAY - 1000, 0));

  return exit_status();
}
