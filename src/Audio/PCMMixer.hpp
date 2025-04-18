// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMPlayer.hpp"
#include "PCMMixerDataSource.hpp"
#include "thread/Mutex.hxx"

#include <memory>

/**
 * Mixes PCM data from multiple #PCMDataSource instances, using a
 * #PCMMixerDataSource, to be played simultaneously by one #PCMPlayer.
 *
 * All public functions are thread-safe.
 */
class PCMMixer final {
  Mutex lock;

  PCMMixerDataSource mixer_data_source;

  std::unique_ptr<PCMPlayer> player;

public:
  PCMMixer(unsigned sample_rate, std::unique_ptr<PCMPlayer> &&player);
  virtual ~PCMMixer() = default;

  PCMMixer(PCMMixer &) = delete;
  PCMMixer &operator=(PCMMixer &) = delete;

  /**
   * Start playback for a given source
   */
  bool Start(PCMDataSource &source);

  /**
   * Stop playback for a given source
   */
  void Stop(PCMDataSource &source);

  void SetVolume(unsigned vol_percent) {
    mixer_data_source.SetVolume(vol_percent);
  }

  static constexpr unsigned GetMaxSafeVolume() {
    return decltype(mixer_data_source)::GetMaxSafeVolume();
  }
};
