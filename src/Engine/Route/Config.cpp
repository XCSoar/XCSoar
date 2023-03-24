// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Config.hpp"

void
RoutePlannerConfig::SetDefaults()
{
  mode = Mode::NONE; // default disable while experimental
  allow_climb = true;
  use_ceiling = false;
  safety_height_terrain = 150;
  reach_calc_mode = ReachMode::STRAIGHT;
  reach_polar_mode = Polar::SAFETY;
}
