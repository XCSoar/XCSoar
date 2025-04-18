// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMDataSource.hpp"
#include "thread/Mutex.hxx"

#include <cstdint>
#include <list>
#include <span>

/**
 * A #PCMDataSource for playback of memory buffers with raw PCM data.
 *
 * The PCM data must be mono, 16 bit (signed), big endian with 44100 Hz sampe
 * rate.
 */
class PCMBufferDataSource : public PCMDataSource {
public:
  using PCMData = std::span<const int16_t>;

private:
  Mutex lock;

  std::list<PCMData> queued_data;
  unsigned offset;

public:
  /**
   * Add a #PCMData buffer to the queue.
   *
   * @return Count of queued #PCMData buffers (including the added one).
   */
  unsigned Add(PCMData &&data);

  /**
   * Clear the queue.
   */
  void Clear();

  /* virtual methods from class PCMDataSource */
  bool IsBigEndian() const override {
    return false; // Our PCM resources are always little endian
  }

  unsigned GetSampleRate() const override {
    return 44100; // Our PCM resources have this sample rate
  }

  size_t GetData(int16_t *buffer, size_t n) override;
};
