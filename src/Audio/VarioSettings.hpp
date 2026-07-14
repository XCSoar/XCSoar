// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <type_traits>

enum class VarioSoundSwitchingMode : uint8_t {
  MANUAL,
  AUTO,
};

enum class VarioSoundManualMode : uint8_t {
  VARIO,
  STF,
};

struct VarioSoundSettings {
  bool enabled;
  uint8_t volume;
  VarioSoundSwitchingMode switching_mode;
  VarioSoundManualMode manual_mode;
  bool dead_band_enabled;

  unsigned min_frequency;
  unsigned zero_frequency;
  unsigned max_frequency;

  unsigned min_period_ms;
  unsigned max_period_ms;

  double min_dead;
  double max_dead;

  void SetDefaults();
};

static_assert(std::is_trivial<VarioSoundSettings>::value, "type is not trivial");
