// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Stamp.hpp"
#include "Math/Filter.hpp"
#include "Math/AvFilter.hpp"
#include "Math/DiffFilter.hpp"

class DistanceStat;

/**
 * Calculate incremental speed from consecutive distance values.
 */
class IncrementalSpeedComputer {
  static constexpr unsigned N_AV = 3;

  AvFilter<N_AV> av_dist;
  DiffFilter df;
  Filter v_lpf;
  const bool is_positive;

  TimeStamp last_time;

public:
  /** Constructor; initialises all to zero */
  IncrementalSpeedComputer(const bool is_positive=true);

  /**
   * Calculate incremental speed from previous step.
   * Resets incremental speed to speed if dt=0
   *
   * @param time monotonic time of day in seconds
   */
  void Compute(DistanceStat &data, TimeStamp time) noexcept;

  void Reset(DistanceStat &data);
};
