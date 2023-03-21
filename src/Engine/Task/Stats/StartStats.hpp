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
  bool task_started;

  /**
   * The time when the task was started [UTC seconds of day].  Only
   * valid if #task_started is true.
   */
  TimeStamp time;

  /**
   * The aircraft's altitude when the task was started [m MSL].  Only
   * valid if #task_started is true.
   */
  double altitude;

  /**
   * The aircraft's ground speed when the task was started [m/s].
   * Only valid if #task_started is true.
   */
  double ground_speed;

  void Reset() {
    task_started = false;
  }

  /**
   * Enable the #task_started flag and copy data from the
   * #AircraftState.
   */
  void SetStarted(const AircraftState &aircraft);
};

static_assert(std::is_trivial<StartStats>::value, "type is not trivial");
