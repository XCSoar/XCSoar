// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMPlayer.hpp"

#include <SDL3/SDL_audio.h>

/**
 * PCMPlayer implementation based on SDL audio streams.
 */
class SDLPCMPlayer : public PCMPlayer {
  SDL_AudioStream *stream = nullptr;

  /** Protected by the stream lock, including inside the callback. */
  bool exhausted = false;

  static void SDLCALL AudioCallback(void *ctx, SDL_AudioStream *stream,
                                   int additional_amount, int total_amount);

public:
  SDLPCMPlayer() = default;
  ~SDLPCMPlayer() override;

  bool Start(PCMDataSource &source) override;
  void Stop() override;
};
