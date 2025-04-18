// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"

void
SoundSettings::SetDefaults()
{
  sound_task_enabled = true;
  sound_modes_enabled = true;
  sound_deadband = 5;

  master_volume = 50;

  vario.SetDefaults();
}
