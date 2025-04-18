// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <cstdint>

#include "PCMDataSource.hpp"
#include "util/ByteOrder.hxx"

class PCMSynthesiser : public PCMDataSource {
public:
  /**
   * The caller requests to generate PCM samples.
   *
   * @param buffer the destination buffer (host byte order)
   * @param n the number of 16 bit mono samples that shall be generated
   */
  virtual void Synthesise(int16_t *buffer, size_t n) = 0;

  /* virtual methods from class PCMDataSource */

  bool IsBigEndian() const {
    /* Our PCM synthesisers always deliver data in host byte order */
    return ::IsBigEndian();
  }

  size_t GetData(int16_t *buffer, size_t n) {
    Synthesise(buffer, n);
    return n;
  }
};
