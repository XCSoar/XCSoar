// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"

void
ContestSettings::SetDefaults() noexcept
{
  enable = true;
  predict = false;
  contest = Contest::WEGLIDE_FREE;
  handicap = 100;
}
