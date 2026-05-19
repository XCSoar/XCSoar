// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMPlayer.hpp"

#include <SDL_audio.h>

#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * PCMPlayer implementation based on SDL
 */
class SDLPCMPlayer : public PCMPlayer {
  SDL_AudioDeviceID device = -1;
  SDL_AudioFormat format = AUDIO_S16SYS;
  std::vector<int16_t> convert_buffer;

  inline void AudioCallback(Uint8 *stream, size_t len_bytes);
  inline size_t AudioCallback(int16_t *stream, size_t len_bytes);
  inline size_t AudioCallback(float *stream, size_t len_bytes);

public:
  SDLPCMPlayer() = default;
  virtual ~SDLPCMPlayer();

  /* virtual methods from class PCMPlayer */
  bool Start(PCMDataSource &source) override;
  void Stop() override;
};
