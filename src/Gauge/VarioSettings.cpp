// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioSettings.hpp"

void
VarioSettings::SetDefaults() noexcept
{
  show_average = true;
  show_mc = true;
  show_speed_to_fly = true;
  show_ballast = true;
  show_bugs = true;
  show_gross = true;
  show_average_needle = false;
  show_thermal_average_needle = true;
}
