// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenDateTime.hpp"
#include "system/Path.hpp"

/**
 * Fallback takeoff-speed threshold [m/s] when no valid polar is
 * configured (same default as GlideComputerAirData).
 */
constexpr double DEFAULT_IGC_TAKEOFF_SPEED = 10;

/**
 * Takeoff / landing times detected from an IGC track using the same
 * #FlyingComputer rules as in-flight detection.  Manufacturer E-records
 * (TKOFF / LAND) are ignored so older logs without those markers still
 * work.
 */
struct IGCFlightTimesResult {
  BrokenDateTime takeoff_utc = BrokenDateTime::Invalid();
  BrokenDateTime landing_utc = BrokenDateTime::Invalid();

  /**
   * True when the file has less than 5 s below the takeoff-speed
   * threshold before the detected takeoff (logger likely started
   * late).
   */
  bool started_too_late = false;

  /**
   * True when the file has less than 5 s below the takeoff-speed
   * threshold after the detected landing, or the logger stopped
   * while still airborne.
   */
  bool ended_too_early = false;

  [[nodiscard]] [[gnu::pure]]
  bool IsValid() const noexcept {
    return takeoff_utc.IsPlausible() && landing_utc.IsPlausible();
  }
};

/**
 * Scan @p path for GPS B-records and detect takeoff/landing.
 *
 * @param takeoff_speed threshold in m/s (use polar GetVTakeoff() or
 *        #DEFAULT_IGC_TAKEOFF_SPEED when no polar is available)
 * @return true when both takeoff and landing were found
 */
bool
DetectIGCFlightTimes(Path path, double takeoff_speed,
                     IGCFlightTimesResult &result) noexcept;
