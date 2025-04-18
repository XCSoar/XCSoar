// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "IncrementalSpeedComputer.hpp"

class DistanceStat;

/**
 * Computer class for DistanceStat.  It holds the incremental and
 * internal values, while DistanceStat has only the results.
 */
class DistanceStatComputer {
  IncrementalSpeedComputer incremental_speed;

public:
  /** Constructor; initialises all to zero */
  DistanceStatComputer(const bool is_positive=true)
    :incremental_speed(is_positive) {}

  /**
   * Calculate bulk speed (distance/time), abstract base method
   *
   * @param es ElementStat (used for time access)
   */
  void CalcSpeed(DistanceStat &data, FloatDuration time) noexcept;

  /**
   * Calculate incremental speed from previous step.
   * Resets incremental speed to speed if dt=0
   *
   * @param time monotonic time of day in seconds
   */
  void CalcIncrementalSpeed(DistanceStat &data, TimeStamp time) noexcept {
    incremental_speed.Compute(data, time);
  }

  void ResetIncrementalSpeed(DistanceStat &data) {
    incremental_speed.Reset(data);
  }
};
