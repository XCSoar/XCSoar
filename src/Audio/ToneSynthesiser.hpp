// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PCMSynthesiser.hpp"

/**
 * This class generates tones with a sine wave.
 */
class ToneSynthesiser : public PCMSynthesiser {
  unsigned volume = 100, angle = 0, increment = 0;

public:
  explicit ToneSynthesiser(unsigned _sample_rate) : sample_rate(_sample_rate) {
  }

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
