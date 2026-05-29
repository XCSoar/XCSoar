// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
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
 * an airspace. Entry is the nearer boundary (lower distance),
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

/** Minimum fragment length (meters) kept by SubtractInterval. */
static constexpr double kMinFragmentLength = 50.0;

/**
 * Subtract a cleared interval from a target interval in-place.
 *
 * Five cases:
 * 1. No overlap: target unchanged
 * 2. Cleared clips start: target.entry = cleared.exit
 * 3. Cleared clips end: target.exit = cleared.entry
 * 4. Cleared fully covers: target = Invalid
 * 5. Cleared creates hole: keep nearest fragment if >= tolerance,
 *    else keep farther fragment if >= tolerance, else Invalid
 *
 * @param target Modified in-place; set to Invalid if fully covered
 * @param cleared The cleared interval to subtract
 * @param tolerance Minimum fragment length to keep nearest (meters)
 */
inline void
SubtractInterval(AirspaceWarningInterval &target,
                 const AirspaceWarningInterval &cleared,
                 double tolerance = kMinFragmentLength) noexcept
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

  /* Keep only the nearest fragment if it meets the tolerance, 
     in which case the the farther fragment is silently dropped.
     this can lead to a delayed NEAR warning (warning will only
     show after aircraft has passed first segment) if the nearer 
     segment is later covered by another clearance, but this is
     acceptable and also an extremely rare case. If first
     fragment is too short, drop it and try the farther one */
  if (nearest.Length() >= tolerance)
    target = nearest;
  else if (farther.Length() >= tolerance)
    target = farther;
  else
    target = AirspaceWarningInterval::Invalid();
}

/**
 * Linear altitude profile along a straight predicted path.
 *
 * Holds the start and end of the predicted path for one
 * prediction method (TASK / FILTER / GLIDE), in both location
 * and altitude. Used to convert a 2D-projected interval into a
 * 3D-aware one: along a straight path with linear vertical rate,
 * the aircraft's altitude is a linear function of distance.
 *
 * Linear is a deliberate approximation; the pre-existing predicted-
 * warning intercept code uses the same model internally.
 */
struct PathAltitudeProfile {
  GeoPoint start_location;
  GeoPoint end_location;
  /** Total straight-line distance, metres. */
  double total_distance;
  /** Aircraft altitude at start_location (m MSL). */
  double start_altitude;
  /** Predicted altitude at end_location (m MSL). */
  double end_altitude;

  [[gnu::pure]]
  bool IsFlat() const noexcept {
    return std::abs(end_altitude - start_altitude) < 1e-3;
  }

  [[gnu::pure]]
  double AltitudeAt(double d) const noexcept {
    if (total_distance <= 0)
      return start_altitude;
    return start_altitude +
      (end_altitude - start_altitude) * (d / total_distance);
  }

  /**
   * Distance along the path at which altitude reaches @p alt.
   * Caller must ensure !IsFlat().
   */
  [[gnu::pure]]
  double DistanceAtAltitude(double alt) const noexcept {
    if (std::fabs(total_distance) < 1e-12)
      return (end_altitude == start_altitude)
        ? 0.0
        : std::numeric_limits<double>::quiet_NaN();
    const double slope =
      (end_altitude - start_altitude) / total_distance;
    return (alt - start_altitude) / slope;
  }

  [[gnu::pure]]
  GeoPoint LocationAt(double d) const noexcept {
    if (total_distance <= 0)
      return start_location;
    return start_location.Interpolate(end_location,
                                      d / total_distance);
  }
};

/**
 * Clip a 2D-projected path interval to the sub-segment along
 * which the aircraft's altitude profile is inside the band
 * [band_base, band_top].  Returns Invalid() if the clipped
 * segment is empty.
 *
 * Because altitude is linear (monotonic) in distance along the
 * straight predicted path, the set { d : alt(d) in band } is a
 * single contiguous interval, so the result is well-defined.
 */
[[gnu::pure]]
inline AirspaceWarningInterval
ClipByAltitudeBand(const AirspaceWarningInterval &iv,
                   const PathAltitudeProfile &alt,
                   double band_base,
                   double band_top) noexcept
{
  if (!iv.IsValid())
    return AirspaceWarningInterval::Invalid();

  if (alt.IsFlat()) {
    const double a = alt.start_altitude;
    if (a < band_base || a > band_top)
      return AirspaceWarningInterval::Invalid();
    return iv;
  }

  /* Degenerate path: near-zero horizontal distance but non-flat
     altitude (IsFlat() already passed). DistanceAtAltitude()
     would return NaN (slope = alt / ~0). The path is a single
     point; use start_altitude as the representative altitude,
     similar to IsFlat() logic. */
  if (std::fabs(alt.total_distance) < 1e-12) {
    if (alt.start_altitude < band_base || alt.start_altitude > band_top)
      return AirspaceWarningInterval::Invalid();
    return iv;
  }

  double d_band_min, d_band_max;
  if (alt.start_altitude > alt.end_altitude) {
    // Descending: alt decreases as d grows.
    d_band_min = alt.DistanceAtAltitude(band_top);
    d_band_max = alt.DistanceAtAltitude(band_base);
  } else {
    // Climbing: alt increases as d grows.
    d_band_min = alt.DistanceAtAltitude(band_base);
    d_band_max = alt.DistanceAtAltitude(band_top);
  }

  const double new_entry =
    std::max(iv.entry.distance, d_band_min);
  const double new_exit =
    std::min(iv.exit.distance, d_band_max);

  if (new_entry >= new_exit)
    return AirspaceWarningInterval::Invalid();

  AirspaceWarningInterval out;
  out.entry.distance = new_entry;
  out.exit.distance = new_exit;
  out.entry.location = (new_entry == iv.entry.distance)
    ? iv.entry.location
    : alt.LocationAt(new_entry);
  out.exit.location = (new_exit == iv.exit.distance)
    ? iv.exit.location
    : alt.LocationAt(new_exit);
  return out;
}
