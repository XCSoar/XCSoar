// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Contests.hpp"
#include "util/Macros.hpp"

static const char *const contest_to_string[] = {
  "OLC Sprint",
  "OLC FAI",
  "OLC Classic",
  "OLC League",
  "OLC Plus",
  "XContest",
  "DHV-XC",
  "SIS-AT",
  "DMSt",
  "WeGlide FREE",
  "WeGlide Distance",
  "WeGlide FAI",
  "WeGlide O&R",
  "Charron",
  "None",
};

const char*
ContestToString(Contest contest) noexcept
{
  unsigned i = (unsigned)contest;
  return i < ARRAY_SIZE(contest_to_string)
    ? contest_to_string[i]
    : nullptr;
}
