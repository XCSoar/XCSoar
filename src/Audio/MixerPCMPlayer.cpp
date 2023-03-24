// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MixerPCMPlayer.hpp"

#include "GlobalPCMMixer.hpp"
#include "PCMMixer.hpp"

MixerPCMPlayer::~MixerPCMPlayer()
{
  Stop();
}

bool
MixerPCMPlayer::Start(PCMDataSource &_source)
{
  Stop();

  if (pcm_mixer->Start(_source)) {
    source = &_source;
    return true;
  } else {
    return false;
  }
}

void
MixerPCMPlayer::Stop()
{
  if (nullptr != source) {
    pcm_mixer->Stop(*source);
    source = nullptr;
  }
}
