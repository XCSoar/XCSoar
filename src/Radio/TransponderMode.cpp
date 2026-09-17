// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TransponderMode.hpp"
#include "Language/Language.hpp"
#include "util/Macros.hpp"

static const char *const mode_strings[] = {
  "",
  "OFF",
  "SBY",
  "GND",
  "ON",
  "ALT",
  "IDENT",
};

const char *
TransponderMode::ToString(Mode mode) noexcept
{
  unsigned i = (unsigned)mode;
  return i < ARRAY_SIZE(mode_strings)
    ? mode_strings[i]
    : "Unknown";
}
