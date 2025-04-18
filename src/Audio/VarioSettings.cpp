// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioSettings.hpp"

void
VarioSoundSettings::SetDefaults()
{
  enabled = false;
  volume = 80;
  dead_band_enabled = false;

  min_frequency = 200;
  zero_frequency = 500;
  max_frequency = 1500;

  min_period_ms = 150;
  max_period_ms = 600;

  min_dead = -0.3;
  max_dead = 0.1;
}
