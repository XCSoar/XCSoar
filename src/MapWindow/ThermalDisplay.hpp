// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;
struct SpeedVector;
struct ThermalSource;

namespace ThermalDisplay {

static constexpr double MAX_MAP_SCALE = 4000;

/** Whether thermal history icons and map items are visible at this scale. */
static constexpr bool
IsVisible(double map_scale) noexcept
{
  return map_scale <= MAX_MAP_SCALE;
}

/**
 * Project an ownship thermal source to the aircraft altitude, or return an
 * invalid location when the aircraft is below the source.
 */
[[nodiscard]] [[gnu::pure]]
GeoPoint
GetLocation(const ThermalSource &source, double aircraft_altitude,
            const SpeedVector &wind) noexcept;

} // namespace ThermalDisplay
