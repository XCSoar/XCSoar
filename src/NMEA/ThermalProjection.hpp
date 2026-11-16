// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;
struct SpeedVector;

/**
 * Convert a wind vector and positive lift rate to horizontal drift per metre
 * of altitude.  Invalid inputs and non-positive lift produce zero drift.
 */
[[nodiscard]] [[gnu::pure]]
SpeedVector
CalculateThermalDriftPerMeter(const SpeedVector &wind,
                              double lift_rate) noexcept;

/**
 * Project a thermal core through a vertical height delta using horizontal
 * drift per metre.  Positive deltas move upwind; negative deltas move
 * downwind.
 */
[[nodiscard]] [[gnu::pure]]
GeoPoint
ProjectThermalCore(const GeoPoint &location, double height_delta,
                   const SpeedVector &drift_per_meter) noexcept;

/**
 * Convenience entry point which derives drift per metre from wind and lift.
 */
[[nodiscard]] [[gnu::pure]]
GeoPoint
ProjectThermalCore(const GeoPoint &location, double height_delta,
                   const SpeedVector &wind, double lift_rate) noexcept;
