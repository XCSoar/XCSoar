// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMBufferDataSource.hpp"
#include "PCMDataSource.hpp"
#include "PCMPlayer.hpp"
#include "thread/Mutex.hxx"

#include <memory>

/**
 * Can play sounds, stored as raw PCM in an embedded resource, using #PCMPlayer
 */
class PCMResourcePlayer {
  Mutex lock;

  std::unique_ptr<PCMPlayer> player;

  PCMBufferDataSource buffer_data_source;

public:
  PCMResourcePlayer();
  virtual ~PCMResourcePlayer() = default;

  PCMResourcePlayer(PCMResourcePlayer &) = delete;
  PCMResourcePlayer &operator=(PCMResourcePlayer &) = delete;

  bool PlayResource(const char *resource_name);
};
