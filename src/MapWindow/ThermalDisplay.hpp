// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;
struct SpeedVector;
struct ThermalSource;
struct TrafficThermalSource;

namespace ThermalDisplay {

static constexpr double MAX_MAP_SCALE = 4000;

/** Whether thermal history icons and map items are visible at this scale. */
static constexpr bool
IsVisible(double map_scale) noexcept
{
  return map_scale <= MAX_MAP_SCALE;
}

/** The global visibility gates specific to FLARM thermal markers. */
static constexpr bool
IsTrafficVisible(bool show_flarm_on_map, double map_scale) noexcept
{
  return show_flarm_on_map && IsVisible(map_scale);
}

/**
 * Project an ownship thermal source to the aircraft altitude, or return an
 * invalid location when the aircraft is below the source.
 */
[[nodiscard]] [[gnu::pure]]
GeoPoint
GetLocation(const ThermalSource &source, double aircraft_altitude,
            const SpeedVector &wind) noexcept;

/**
 * Project a FLARM thermal source to the aircraft altitude, or return an
 * invalid location when the aircraft is below the source.
 */
[[nodiscard]] [[gnu::pure]]
GeoPoint
GetLocation(const TrafficThermalSource &source,
            double aircraft_altitude) noexcept;

} // namespace ThermalDisplay
