// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/SpeedVector.hpp"
#include "Math/Angle.hpp"
#include "time/Stamp.hpp"

/**
 * GPS-only wind estimate for discussion #2918 (prototype).
 *
 * Ground speed versus ground track looks like a cosine: fastest over
 * the ground is downwind, slowest is upwind.  Split the compass into
 * 72 bins of 5°, keep the latest GPS speed in each bin, then fit
 *
 *   ground_speed(track) ≈ tas + wind * cos(track - downwind)
 *
 * That is the usual light-wind approximation.  It does not need a
 * detected thermal or a round GPS circle — only a spread of tracks.
 */
class BucketWind {
  static constexpr unsigned N_BINS = 72;
  static constexpr double BIN_WIDTH_DEG = 360. / N_BINS;

  struct Bin {
    TimeStamp time = TimeStamp::Undefined();
    double ground_speed = 0;
  };

  Bin bins[N_BINS];

  [[gnu::const]]
  static unsigned BinIndex(Angle track) noexcept;

  [[gnu::const]]
  static Angle BinCentre(unsigned i) noexcept;

public:
  struct Result {
    bool valid = false;
    /** Meteorological wind (direction FROM). */
    SpeedVector wind = SpeedVector::Zero();
    /** Mean ground speed of the fit; ≈ TAS when wind is light. */
    double tas = 0;
    unsigned n_bins = 0;
  };

  void Reset() noexcept;

  void Update(TimeStamp time, Angle track,
              double ground_speed) noexcept;

  /**
   * Fit wind from bins that are still younger than #max_age.
   */
  [[nodiscard]]
  Result Fit(TimeStamp now,
             FloatDuration max_age = FloatDuration{180}) const noexcept;
};
