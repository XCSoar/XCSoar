// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMSynthesiser.hpp"

#include <algorithm>

/**
 * This class generates tones with a sine wave.
 */
class ToneSynthesiser : public PCMSynthesiser {
  unsigned volume = 100;
  unsigned angle = 0;
  unsigned increment = 0, target_increment = 0;

  /**
   * Higher value means slower pitch changes.
   *
   * This smoothes the tone when input updates are low-rate/noisy
   * (e.g. GPS vario).
   */
  unsigned slew_divisor;

public:
  explicit ToneSynthesiser(unsigned _sample_rate)
    :slew_divisor(std::max(64u, _sample_rate / 4u)),
     sample_rate(_sample_rate) {}

  unsigned GetSampleRate() const {
    return sample_rate;
  }

  /**
   * Set the (software) volume of the generated tone.
   *
   * @param _volume the new volume level, 0 indicating muted, 100
   * means full volume
   */
  void SetVolume(unsigned _volume) {
    volume = _volume;
  }

  void SetTone(unsigned tone_hz);

  /* methods from class PCMSynthesiser */
  virtual void Synthesise(int16_t *buffer, size_t n);

protected:
  const unsigned sample_rate;

  /**
   * Returns the number of samples until the sample value gets close
   * to zero.
   */
  [[gnu::pure]]
  unsigned ToZero() const;

  /**
   * Start a new period.
   */
  void Restart() {
    angle = 0;
  }
};
