// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;
struct SpeedVector;
struct ThermalSource;
struct TrafficThermalSource;
struct MapLook;
class MaskedIcon;

namespace ThermalDisplay {

static constexpr double MAX_MAP_SCALE = 4000;

enum class TrafficLiftCategory {
  BETTER,
  WITHIN_TEN_PERCENT,
  WORSE,
};

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
 * Compare a FLARM thermal's reported lift with the current MacCready setting.
 * Lift above MacCready is better; otherwise, lift down to 10% below
 * MacCready is near.
 */
[[nodiscard]] [[gnu::const]]
TrafficLiftCategory
ClassifyTrafficLift(double lift_rate, double mac_cready) noexcept;

/**
 * Select the FLARM thermal icon for the reported lift and current
 * MacCready setting.
 */
[[nodiscard]] [[gnu::pure]]
const MaskedIcon &
GetFlarmThermalIcon(const MapLook &look, double lift_rate,
                    double mac_cready) noexcept;

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
