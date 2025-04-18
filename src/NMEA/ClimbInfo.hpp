// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Stamp.hpp"

#include <type_traits>

/** Information about one thermal that was or is being climbed. */
struct OneClimbInfo
{
  /** Time when circling started. */
  TimeStamp start_time;

  /**
   * Time when circling ended
   * (or current time stamp if circling has not ended yet).
   */
  TimeStamp end_time;

  /** Time spent in this thermal [s]. */
  FloatDuration duration;

  /** Altitude gained while in the thermal [m]. May be negative. */
  double gain;

  /** Start altitude [m] */
  double start_altitude;

  /** Average vertical speed in the thermal [m/s]. May be negative. */
  double lift_rate;

  void Clear();

  bool IsDefined() const {
    return duration.count() > 0;
  }

  void CalculateDuration() {
    duration = end_time - start_time;
  }

  void CalculateLiftRate() {
    lift_rate = IsDefined()
      ? gain / duration.count()
      : 0.;
  }

  void CalculateAll() {
    CalculateDuration();
    CalculateLiftRate();
  }
};

/** Derived climb data */
struct ClimbInfo
{
  OneClimbInfo current_thermal;

  OneClimbInfo last_thermal;

  /** Average vertical speed in the last thermals smoothed by low-pass-filter */
  double last_thermal_average_smooth;

  void Clear();
};

static_assert(std::is_trivial<ClimbInfo>::value, "type is not trivial");
