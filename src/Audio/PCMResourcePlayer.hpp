// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMBufferDataSource.hpp"
#include "PCMDataSource.hpp"
#include "PCMPlayer.hpp"
#include "thread/Mutex.hxx"

#include <memory>
#include <span>

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

  /**
   * Play the resources one after another, instead of what is still
   * queued or playing.
   */
  bool PlayResources(std::span<const char *const> resource_names);
};
