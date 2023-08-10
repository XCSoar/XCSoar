// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Stamp.hpp"

#include <type_traits>

struct AircraftState;

/**
 * Container for start point statistics.
 */
struct StartStats {
  /**
   * The time when the task was started [UTC seconds of day].  Only
   * valid if HasStarted() is true.
   */
  TimeStamp time;

  /**
   * The aircraft's altitude when the task was started [m MSL].  Only
   * valid if HasStarted() is true.
   */
  double altitude;

  /**
   * The aircraft's ground speed when the task was started [m/s].
   * Only valid if HasStarted() is true.
   */
  double ground_speed;

  constexpr void Reset() noexcept {
    time = TimeStamp::Undefined();
  }

  bool HasStarted() const noexcept {
    return time.IsDefined();
  }

  /**
   * Enable the HasStarted() flag and copy data from the
   * #AircraftState.
   */
  void SetStarted(const AircraftState &aircraft) noexcept;

  TimeStamp GetStartedTime() const noexcept {
    return time;
  }
};

static_assert(std::is_trivial<StartStats>::value, "type is not trivial");
