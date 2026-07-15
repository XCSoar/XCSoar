// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioAudioValue.hpp"

#include <algorithm>
#include <cmath>

namespace AudioVarioGlue {

static constexpr double VARIO_FULL_SCALE = 5.0;
static constexpr double STF_SPEED_DEAD_BAND = 2.0;

[[gnu::const]]
static double
STFSpeedErrorToPseudoVario(double speed_error) noexcept
{
  /* Square-root scaling compresses large speed errors while preserving
     useful cue separation just outside the quiet band.  Sink-like tones
     command speeding up; climb-like tones command slowing down. */
  const double cue = -std::copysign(std::sqrt(std::abs(speed_error)),
                                    speed_error);
  return std::clamp(cue,
                    -VARIO_FULL_SCALE, VARIO_FULL_SCALE);
}

std::optional<double>
ComputeAudioValue(const VarioAudioInput &input,
                  const VarioSoundSwitchingMode switching_mode,
                  const VarioSoundManualMode manual_mode) noexcept
{
  const bool use_stf = switching_mode == VarioSoundSwitchingMode::AUTO
    ? !input.circling
    : manual_mode == VarioSoundManualMode::STF;

  if (!use_stf)
    return input.vario;

  if (input.stf_speed_error) {
    if (std::abs(*input.stf_speed_error) <= STF_SPEED_DEAD_BAND)
      return std::nullopt;

    return STFSpeedErrorToPseudoVario(*input.stf_speed_error);
  }

  /* Automatic switching keeps the vario useful until STF inputs become
     available.  Explicit manual STF instead stays silent. */
  return switching_mode == VarioSoundSwitchingMode::AUTO
    ? input.vario
    : std::nullopt;
}

} // namespace AudioVarioGlue
