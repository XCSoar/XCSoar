/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "VarioGlue.hpp"
#include "PCMPlayer.hpp"
#include "VarioSynthesiser.hpp"
#include "VarioSettings.hpp"

#ifdef ANDROID
#include "SLES/Init.hpp"
#endif

#include <assert.h>

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
  assert(player == NULL);
  assert(synthesiser == NULL);

#ifdef ANDROID
  have_sles = SLES::Initialise();
  if (!have_sles)
    return;
#endif

  player = new PCMPlayer();
  synthesiser = new VarioSynthesiser();
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

  assert(player != NULL);
  assert(synthesiser != NULL);

  if (settings.enabled) {
    synthesiser->SetVolume(settings.volume);
    synthesiser->SetDeadBand(settings.dead_band_enabled);
    synthesiser->SetFrequencies(settings.min_frequency, settings.zero_frequency,
                                settings.max_frequency);
    synthesiser->SetPeriods(settings.min_period_ms, settings.max_period_ms);
    synthesiser->SetDeadBandRange(settings.min_dead, settings.max_dead);
    player->Start(*synthesiser, sample_rate);
  } else
    player->Stop();
}

void
AudioVarioGlue::SetValue(fixed vario)
{
#ifdef ANDROID
  if (!have_sles)
    return;
#endif

  assert(player != NULL);
  assert(synthesiser != NULL);

  synthesiser->SetVario(sample_rate, vario);
}

void
AudioVarioGlue::NoValue()
{
#ifdef ANDROID
  if (!have_sles)
    return;
#endif

  assert(player != NULL);
  assert(synthesiser != NULL);

  synthesiser->SetSilence();
}
