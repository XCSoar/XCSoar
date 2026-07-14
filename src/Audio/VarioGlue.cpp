// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioGlue.hpp"
#include "PCMPlayer.hpp"
#include "PCMPlayerFactory.hpp"
#include "VarioSynthesiser.hpp"
#include "VarioSettings.hpp"

#ifdef ANDROID
#include "SLES/Init.hpp"
#endif

#include <algorithm>
#include <atomic>
#include <cassert>

static constexpr unsigned sample_rate = 44100;

#ifdef ANDROID
static bool have_sles;
#endif

static PCMPlayer *player;
static VarioSynthesiser *synthesiser;
static std::atomic<VarioSoundSwitchingMode> switching_mode{
  VarioSoundSwitchingMode::MANUAL
};
static std::atomic<VarioSoundManualMode> manual_mode{
  VarioSoundManualMode::VARIO
};

static constexpr double stf_full_scale = 16.0;
static constexpr double vario_full_scale = 5.0;

[[gnu::const]]
static double
STFSpeedErrorToPseudoVario(double speed_error) noexcept
{
  /* Reuse the existing tone model: sink-like tones command speeding up,
     climb-like tones command slowing down. */
  return std::clamp(-speed_error * (vario_full_scale / stf_full_scale),
                    -vario_full_scale, vario_full_scale);
}

bool
AudioVarioGlue::HaveAudioVario()
{
#ifdef ANDROID
  return have_sles;
#else
  return true;
#endif
}

void
AudioVarioGlue::Initialise()
{
  assert(player == nullptr);
  assert(synthesiser == nullptr);

#ifdef ANDROID
  have_sles = SLES::Initialise();
  if (!have_sles)
    return;
#endif

  player = PCMPlayerFactory::CreateInstance();
  synthesiser = new VarioSynthesiser(sample_rate);
}

void
AudioVarioGlue::Deinitialise()
{
  delete player;
  player = nullptr;
  delete synthesiser;
  synthesiser = nullptr;
}

void
AudioVarioGlue::Configure(const VarioSoundSettings &settings)
{
#ifdef ANDROID
  if (!have_sles)
    return;
#endif

  assert(player != nullptr);
  assert(synthesiser != nullptr);

  manual_mode = settings.manual_mode;
  switching_mode = settings.switching_mode;

  if (settings.enabled) {
    synthesiser->SetVolume(settings.volume);
    synthesiser->SetDeadBand(settings.dead_band_enabled);
    synthesiser->SetFrequencies(settings.min_frequency, settings.zero_frequency,
                                settings.max_frequency);
    synthesiser->SetPeriods(settings.min_period_ms, settings.max_period_ms);
    synthesiser->SetDeadBandRange(settings.min_dead, settings.max_dead);
    player->Start(*synthesiser);
  } else
    player->Stop();
}

void
AudioVarioGlue::SetValue(const VarioAudioInput &input)
{
#ifdef ANDROID
  if (!have_sles)
    return;
#endif

  assert(player != nullptr);
  assert(synthesiser != nullptr);

  const auto configured_switching_mode =
    switching_mode.load();
  const bool use_stf = configured_switching_mode == VarioSoundSwitchingMode::AUTO
    ? !input.circling
    : manual_mode.load() == VarioSoundManualMode::STF;

  if (use_stf) {
    if (input.stf_speed_error)
      synthesiser->SetVario(STFSpeedErrorToPseudoVario(*input.stf_speed_error));
    else if (configured_switching_mode == VarioSoundSwitchingMode::AUTO &&
             input.vario)
      synthesiser->SetVario(*input.vario);
    else
      synthesiser->SetSilence();

    return;
  }

  if (input.vario)
    synthesiser->SetVario(*input.vario);
  else
    synthesiser->SetSilence();
}
