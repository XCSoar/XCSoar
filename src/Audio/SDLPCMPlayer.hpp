// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMPlayer.hpp"

#include <SDL_audio.h>

/**
 * PCMPlayer implementation based on SDL
 */
class SDLPCMPlayer : public PCMPlayer {
  SDL_AudioDeviceID device = -1;

  inline void AudioCallback(int16_t *stream, size_t len);

public:
  SDLPCMPlayer() = default;
  virtual ~SDLPCMPlayer();

  /* virtual methods from class PCMPlayer */
  bool Start(PCMDataSource &source) override;
  void Stop() override;
};
