// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Audio/VarioAudioValue.hpp"
#include "Audio/VarioSynthesiser.hpp"
#include "Computer/STF.hpp"
#include "Computer/Settings.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "TestUtil.hpp"

#include <algorithm>
#include <array>

using namespace AudioVarioGlue;

static bool
HasSound(const std::optional<double> value) noexcept
{
  VarioSynthesiser synthesiser(44100);
  if (value)
    synthesiser.SetVario(*value);
  else
    synthesiser.SetSilence();

  std::array<int16_t, 2048> buffer{};
  synthesiser.Synthesise(buffer.data(), buffer.size());
  return std::any_of(buffer.begin(), buffer.end(),
                     [](const int16_t sample) { return sample != 0; });
}

static void
TestModesAndDeadBand() noexcept
{
  VarioAudioInput input;
  input.vario = 1.5;

  auto value = ComputeAudioValue(input, VarioSoundSwitchingMode::MANUAL,
                                 VarioSoundManualMode::VARIO);
  ok1(value && equals(*value, 1.5));

  value = ComputeAudioValue(input, VarioSoundSwitchingMode::MANUAL,
                            VarioSoundManualMode::STF);
  ok1(!value);

  input.circling = true;
  value = ComputeAudioValue(input, VarioSoundSwitchingMode::AUTO,
                            VarioSoundManualMode::STF);
  ok1(value && equals(*value, 1.5));

  input.circling = false;
  value = ComputeAudioValue(input, VarioSoundSwitchingMode::AUTO,
                            VarioSoundManualMode::VARIO);
  ok1(value && equals(*value, 1.5));

  input.stf_speed_error = 2.;
  value = ComputeAudioValue(input, VarioSoundSwitchingMode::MANUAL,
                            VarioSoundManualMode::STF);
  ok1(!value);
  ok1(!HasSound(value));

  input.stf_speed_error = -2.;
  value = ComputeAudioValue(input, VarioSoundSwitchingMode::MANUAL,
                            VarioSoundManualMode::STF);
  ok1(!value);

  input.stf_speed_error = 2.01;
  value = ComputeAudioValue(input, VarioSoundSwitchingMode::MANUAL,
                            VarioSoundManualMode::STF);
  ok1(value && *value < 0);
  ok1(HasSound(value));

  input.stf_speed_error = -2.01;
  value = ComputeAudioValue(input, VarioSoundSwitchingMode::MANUAL,
                            VarioSoundManualMode::STF);
  ok1(value && *value > 0);
}

static void
TestMergedSensorSTF() noexcept
{
  ComputerSettings settings;
  settings.features.block_stf_enabled = false;
  settings.task.risk_gamma = 0.;
  settings.polar.glide_polar_task = GlidePolar(1.);

  MoreData basic{};
  basic.clock = TimeStamp{FloatDuration{1}};

  DerivedInfo calculated{};

  ok1(!ComputeSTFSpeedError(basic, calculated, settings));

  calculated.flight.flying = true;
  basic.ProvideBothAirspeeds(35., 35.);
  basic.ProvideTotalEnergyVario(-3.);

  /* This deliberately stale value must not affect the merge-rate result. */
  basic.netto_vario = 100.;
  calculated.V_stf = 35.;
  calculated.V_stf_available = true;

  const auto sink_error = ComputeSTFSpeedError(basic, calculated, settings);
  ok1(sink_error && *sink_error > 4.);

  VarioAudioInput input;
  input.stf_speed_error = sink_error;
  auto value = ComputeAudioValue(input, VarioSoundSwitchingMode::MANUAL,
                                 VarioSoundManualMode::STF);
  ok1(value && *value < 0);
  ok1(HasSound(value));

  /* A new TE sample must reverse the command without a GPS calculation. */
  basic.ProvideTotalEnergyVario(3.);
  const auto lift_error = ComputeSTFSpeedError(basic, calculated, settings);
  ok1(lift_error && *lift_error < -4.);

  input.stf_speed_error = lift_error;
  value = ComputeAudioValue(input, VarioSoundSwitchingMode::MANUAL,
                            VarioSoundManualMode::STF);
  ok1(value && *value > 0);
  ok1(HasSound(value));

  calculated.flight.flying = false;
  ok1(!ComputeSTFSpeedError(basic, calculated, settings));
}

int
main()
{
  plan_tests(18);

  TestModesAndDeadBand();
  TestMergedSensorSTF();

  return exit_status();
}
