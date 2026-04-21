// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <algorithm>
#include <type_traits>

/**
 * A point on an airspace boundary, described by its distance
 * from the current aircraft position and its geographic location.
 */
struct AirspaceIntervalPoint {
  /** Distance from current aircraft position (meters) */
  double distance;

  /** Geographic location of this boundary point */
  GeoPoint location;

  constexpr void SetInvalid() noexcept {
    distance = -1;
  }

  constexpr bool IsValid() const noexcept {
    return distance >= 0;
  }
};

static_assert(std::is_trivially_copyable_v<AirspaceIntervalPoint>);

/**
 * A distance interval along the predicted flight path through
 * an airspace.  entry is the nearer boundary (lower distance),
 * exit is the farther boundary (higher distance).
 *
 * For airspaces the aircraft is currently inside,
 * entry.distance == 0.
 */
struct AirspaceWarningInterval {
  AirspaceIntervalPoint entry;
  AirspaceIntervalPoint exit;

  constexpr bool IsValid() const noexcept {
    return entry.IsValid() && exit.IsValid();
  }

  static constexpr AirspaceWarningInterval Invalid() noexcept {
    AirspaceWarningInterval iv{};
    iv.entry.SetInvalid();
    iv.exit.SetInvalid();
    return iv;
  }

  /**
   * Length of this interval in meters.
   */
  constexpr double Length() const noexcept {
    return exit.distance - entry.distance;
  }
};

static_assert(std::is_trivially_copyable_v<AirspaceWarningInterval>);

/**
 * Subtract a cleared interval from a target interval in-place.
 *
 * Five cases:
 * 1. No overlap: target unchanged
 * 2. Cleared clips start: target.entry = cleared.exit
 * 3. Cleared clips end: target.exit = cleared.entry
 * 4. Cleared fully covers: target = Invalid
 * 5. Cleared creates hole: keep nearest fragment if its
 *    length >= tolerance, otherwise keep the farther fragment
 *
 * @param target Modified in-place; set to Invalid if fully covered
 * @param cleared The cleared interval to subtract
 * @param tolerance Minimum fragment length to keep nearest (meters)
 */
inline void
SubtractInterval(AirspaceWarningInterval &target,
                 const AirspaceWarningInterval &cleared,
                 double tolerance = 50.0) noexcept
{
  if (!target.IsValid() || !cleared.IsValid())
    return;

  // No overlap?
  if (cleared.exit.distance <= target.entry.distance ||
      cleared.entry.distance >= target.exit.distance)
    return;

  // Cleared fully covers target?
  if (cleared.entry.distance <= target.entry.distance &&
      cleared.exit.distance >= target.exit.distance) {
    target = AirspaceWarningInterval::Invalid();
    return;
  }

  // Cleared clips start of target?
  if (cleared.entry.distance <= target.entry.distance) {
    target.entry = cleared.exit;
    return;
  }

  // Cleared clips end of target?
  if (cleared.exit.distance >= target.exit.distance) {
    target.exit = cleared.entry;
    return;
  }

  // Hole in middle: two fragments
  AirspaceWarningInterval nearest = {
    target.entry, cleared.entry
  };
  AirspaceWarningInterval farther = {
    cleared.exit, target.exit
  };

  if (nearest.Length() >= tolerance)
    target = nearest;
  else
    target = farther;
}
