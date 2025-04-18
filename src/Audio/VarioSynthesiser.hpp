// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ToneSynthesiser.hpp"
#include "thread/Mutex.hxx"

/**
 * This class generates vario sound.
 */
class VarioSynthesiser final : public ToneSynthesiser {
  /**
   * This mutex protects all atttributes below.  It is locked
   * automatically by all public methods.
   */
  Mutex mutex;

  /**
   * The number of audible samples in each period.
   */
  size_t audible_count;

  /**
   * The number of silent samples in each period.  If this is zero,
   * then no silence will be generated (continuous tone).
   */
  size_t silence_count;

  /**
   * The number of audible/silence samples remaining in the current
   * period.  These two attributes will be reset to the according
   * _count value when both reach zero.
   */
  size_t audible_remaining, silence_remaining;

  bool dead_band_enabled;

  /**
   * The tone frequency for #min_vario.
   */
  unsigned min_frequency;

  /**
   * The tone frequency for stationary altitude.
   */
  unsigned zero_frequency;

  /**
   * The tone frequency for #max_vario.
   */
  unsigned max_frequency;

  /**
   * The minimum silence+audible period for #max_vario.
   */
  unsigned min_period_ms;

  /**
   * The maximum silence+audible period for #min_vario.
   */
  unsigned max_period_ms;

  /**
   * The vario range of the "dead band" during which no sound is emitted
   * [cm/s].
   */
  int min_dead, max_dead;

public:
  explicit VarioSynthesiser(unsigned sample_rate)
    :ToneSynthesiser(sample_rate),
     audible_count(0), silence_count(1),
     audible_remaining(0), silence_remaining(0),
     dead_band_enabled(false),
     min_frequency(200), zero_frequency(500), max_frequency(1500),
     min_period_ms(150), max_period_ms(600),
     min_dead(-30), max_dead(10) {}

  /**
   * Update the vario value.  This calculates a new tone frequency and
   * a new "silence" rate (for positive vario values).
   *
   * @param vario the current vario value [m/s]
   */
  void SetVario(double vario);

  /**
   * Produce silence from now on.
   */
  void SetSilence();

  /**
   * Enable/disable the dead band silence
   */
  void SetDeadBand(bool enabled) {
    dead_band_enabled = enabled;
  }

  /**
   * Set the base frequencies for minimum, zero and maximum lift
   */
  void SetFrequencies(unsigned min, unsigned zero, unsigned max) {
    min_frequency = min;
    zero_frequency = zero;
    max_frequency = max;
  }

  /**
   * Set the time periods for minimum and maximum lift
   */
  void SetPeriods(unsigned min, unsigned max) {
    min_period_ms = min;
    max_period_ms = max;
  }

  /**
   * Set the vario range of the "dead band" during which no sound is emitted
   */
  void SetDeadBandRange(double min, double max) {
    min_dead = (int)(min * 100);
    max_dead = (int)(max * 100);
  }

  /* methods from class PCMSynthesiser */
  virtual void Synthesise(int16_t *buffer, size_t n);

private:
  /**
   * Same as SetSilence(), but doesn't lock the mutex.
   */
  void UnsafeSetSilence();

  /**
   * Convert a vario value to a tone frequency.
   *
   * @param ivario the current vario value [cm/s]
   */
  [[gnu::const]]
  unsigned VarioToFrequency(int ivario);

  bool InDeadBand(int ivario) {
    return ivario >= min_dead && ivario <= max_dead;
  }
};
