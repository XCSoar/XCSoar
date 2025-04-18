// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * This interface is used by PCMPlayer.
 */
class PCMDataSource {
public:
  /**
   * Check if this data source delivers Big Endian PCM samples.
   *
   * @return true means it delivers Big Endian samples, false means it
   * delivers Little Endian samples
   */
  virtual bool IsBigEndian() const = 0;

  /**
   * Get the sample rate in hz which this PCM data source delivers.
   */
  virtual unsigned GetSampleRate() const = 0;

  /**
   * The caller requests PCM data.
   *
   * @param buffer the destination buffer
   * @param n the number of requested 16 bit mono samples
   *
   * @return the number of delivered samples. If less than n, it means that
   * playback for this data source is to be stopped after playing the returned
   * number of samples.
   */
  virtual size_t GetData(int16_t *buffer, size_t n) = 0;
};
