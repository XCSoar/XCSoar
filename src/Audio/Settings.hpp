// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VarioSettings.hpp"

#include <cstdint>
#include <type_traits>

struct SoundSettings {
  // sound stuff not used?
  bool sound_task_enabled;
  bool sound_modes_enabled;
  uint8_t sound_deadband;

  uint8_t master_volume;

  VarioSoundSettings vario;

  void SetDefaults();
};

static_assert(std::is_trivial<SoundSettings>::value, "type is not trivial");
