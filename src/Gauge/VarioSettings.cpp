// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioSettings.hpp"

void
VarioSettings::SetDefaults() noexcept
{
  show_average = false;
  show_mc = false;
  show_speed_to_fly = false;
  show_ballast = false;
  show_bugs = false;
  show_gross = true;
  show_average_needle = false;
  show_thermal_average_needle = false;
}
