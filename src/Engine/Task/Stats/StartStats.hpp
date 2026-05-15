// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Stamp.hpp"

#include <type_traits>

struct AircraftState;
class TimeSpan;

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

  /**
   * True when #pev_offset_seconds records start time minus PEV window
   * start (#CommonStats::pev_start_time_span) at the crossing.
   */
  bool pev_offset_available;

  /** Seconds from PEV window start to actual start; only if
   * #pev_offset_available. */
  int pev_offset_seconds;

  constexpr void Reset() noexcept {
    time = TimeStamp::Undefined();
    pev_offset_available = false;
    pev_offset_seconds = 0;
  }

  bool HasStarted() const noexcept {
    return time.IsDefined();
  }

  /**
   * Enable the #HasStarted() flag and copy data from the
   * #AircraftState.
   *
   * @param pev_span optional PEV window snapshot; if defined with a
   * valid start, #pev_offset_seconds records start time minus that
   * start.
   */
  void SetStarted(const AircraftState &aircraft,
                  const TimeSpan *pev_span = nullptr) noexcept;

  TimeStamp GetStartedTime() const noexcept {
    return time;
  }
};

static_assert(std::is_trivial<StartStats>::value, "type is not trivial");
