// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VarioSettings.hpp"

#include <optional>

namespace AudioVarioGlue {

struct VarioAudioInput {
  /** Current total-energy vario value [m/s]. */
  std::optional<double> vario;

  /** Whether the aircraft is currently circling. */
  bool circling = false;

  /** Target true airspeed minus current true airspeed [m/s]. */
  std::optional<double> stf_speed_error;
};

/**
 * Convert the current Vario/STF inputs to the value consumed by the vario
 * synthesiser.  An empty result requests silence.
 */
[[gnu::pure]]
std::optional<double>
ComputeAudioValue(const VarioAudioInput &input,
                  VarioSoundSwitchingMode switching_mode,
                  VarioSoundManualMode manual_mode) noexcept;

} // namespace AudioVarioGlue
