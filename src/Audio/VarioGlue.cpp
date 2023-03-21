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

#include <cassert>

static constexpr unsigned sample_rate = 44100;

#ifdef ANDROID
static bool have_sles;
#endif

static PCMPlayer *player;
static VarioSynthesiser *synthesiser;

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
AudioVarioGlue::SetValue(double vario)
{
#ifdef ANDROID
  if (!have_sles)
    return;
#endif

  assert(player != nullptr);
  assert(synthesiser != nullptr);

  synthesiser->SetVario(vario);
}

void
AudioVarioGlue::NoValue()
{
#ifdef ANDROID
  if (!have_sles)
    return;
#endif

  assert(player != nullptr);
  assert(synthesiser != nullptr);

  synthesiser->SetSilence();
}
