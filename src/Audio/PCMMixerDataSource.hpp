// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMDataSource.hpp"
#include "util/ByteOrder.hxx"
#include "thread/Mutex.hxx"

/**
 * #PCMDataSource implementation which mixes PCM data streams from multiple
 * other #PCMDataSource instances to one PCM data stream.
 *
 * Data sources with different byte order can be mixed, but the sample rate
 * must always be the same, because resampling is not supported yet.
 */
class PCMMixerDataSource : public PCMDataSource {
  static constexpr unsigned MAX_MIXER_SOURCES_COUNT = 2;

  const unsigned sample_rate;

  unsigned vol_percent = 100 / MAX_MIXER_SOURCES_COUNT;

  PCMDataSource *sources[MAX_MIXER_SOURCES_COUNT] = {};

  Mutex lock;

public:
  explicit PCMMixerDataSource(unsigned _sample_rate) :
      sample_rate(_sample_rate) {}

  virtual ~PCMMixerDataSource() = default;

  PCMMixerDataSource(PCMMixerDataSource &) = delete;
  PCMMixerDataSource &operator=(PCMMixerDataSource &) = delete;

  /**
   * Add a #PCMDataSource instance for mixing.
   *
   * @return false, if the capacity is exceeded. Otherwise true.
   */
  bool AddSource(PCMDataSource &source);

  /**
   * Remove a #PCMDataSource instance for mixing.
   */
  void RemoveSource(PCMDataSource &source);

  /**
   * Set volume level in percent. The volume for every source is reduced to
   * the given value.
   * When x sources are being played, up to 100/x% are safe, and clipping
   * artifacts can occur for higher levels, see GetMaxSafeVolume().
   */
  void SetVolume(unsigned vol_percent);

  /**
   * Get max volume value which is "safe". No clipping artifacts
   * can occur up to this volume level.
   */
  static constexpr unsigned GetMaxSafeVolume() {
    return 100 / MAX_MIXER_SOURCES_COUNT;
  }

  /* virtual methods from class PCMDataSource */
  bool IsBigEndian() const override {
    return ::IsBigEndian();
  }

  unsigned GetSampleRate() const override {
    return sample_rate;
  }

  size_t GetData(int16_t *buffer, size_t n) override;
};
