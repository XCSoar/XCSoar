// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Audio/SpokenVerticalSpeed.hpp"
#include "Units/Unit.hpp"
#include "TestUtil.hpp"

#include <cstring>
#include <initializer_list>

/**
 * Do the clips for #value say #expected, given as the clip names
 * without the "IDR_WAV_" prefix?
 */
static bool
Says(double value, Unit unit,
     std::initializer_list<const char *> expected) noexcept
{
  const auto clips = GetSpokenVerticalSpeedClips(value, unit);
  if (clips.size() != expected.size())
    return false;

  auto clip = clips.begin();
  for (const char *word : expected) {
    if (strncmp(*clip, "IDR_WAV_", 8) != 0 || strcmp(*clip + 8, word) != 0)
      return false;
    ++clip;
  }

  return true;
}

int
main()
{
  plan_tests(17);

  constexpr Unit ms = Unit::METER_PER_SECOND;
  ok1(Says(1.5, ms, {"NUMBER_1", "POINT", "NUMBER_5_END"}));
  ok1(Says(0, ms, {"NUMBER_0", "POINT", "NUMBER_0_END"}));
  /* no "minus zero point zero" */
  ok1(Says(-0.04, ms, {"NUMBER_0", "POINT", "NUMBER_0_END"}));
  ok1(Says(-0.5, ms, {"MINUS", "NUMBER_0", "POINT", "NUMBER_5_END"}));
  ok1(Says(12.3, ms, {"NUMBER_1", "NUMBER_2", "POINT", "NUMBER_3_END"}));
  ok1(Says(10.0, ms, {"NUMBER_1", "NUMBER_0", "POINT", "NUMBER_0_END"}));
  ok1(Says(1.96, ms, {"NUMBER_2", "POINT", "NUMBER_0_END"}));
  ok1(Says(0.06, ms, {"NUMBER_0", "POINT", "NUMBER_1_END"}));
  ok1(Says(150, ms, {"NUMBER_9", "NUMBER_9", "POINT", "NUMBER_9_END"}));
  ok1(Says(-150, ms,
           {"MINUS", "NUMBER_9", "NUMBER_9", "POINT", "NUMBER_9_END"}));

  ok1(Says(3.2, Unit::KNOTS, {"NUMBER_3", "POINT", "NUMBER_2_END"}));

  constexpr Unit fpm = Unit::FEET_PER_MINUTE;
  ok1(Says(300, fpm, {"NUMBER_3", "HUNDRED"}));
  ok1(Says(1200, fpm, {"NUMBER_1", "THOUSAND", "NUMBER_2", "HUNDRED"}));
  ok1(Says(2000, fpm, {"NUMBER_2", "THOUSAND_END"}));
  ok1(Says(-100, fpm, {"MINUS", "NUMBER_1", "HUNDRED"}));
  ok1(Says(40, fpm, {"NUMBER_0_END"}));
  ok1(Says(20000, fpm,
           {"NUMBER_9", "THOUSAND", "NUMBER_9", "HUNDRED"}));

  return exit_status();
}
