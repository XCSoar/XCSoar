// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMPlayer.hpp"

/**
 * PCMPlayer implementation which uses the #PCMMixer layer
 */
class MixerPCMPlayer : public PCMPlayer {
public:
  MixerPCMPlayer() = default;
  virtual ~MixerPCMPlayer();

  /* virtual methods from class PCMPlayer */
  bool Start(PCMDataSource &source) override;
  void Stop() override;
};
