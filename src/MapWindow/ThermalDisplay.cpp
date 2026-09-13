// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalDisplay.hpp"

#include "Look/MapLook.hpp"
#include "NMEA/ThermalLocator.hpp"
#include "NMEA/TrafficThermal.hpp"

namespace ThermalDisplay {

TrafficLiftCategory
ClassifyTrafficLift(double lift_rate, double mac_cready) noexcept
{
  if (lift_rate > mac_cready)
    return TrafficLiftCategory::BETTER;

  if (lift_rate >= mac_cready * 0.9)
    return TrafficLiftCategory::WITHIN_TEN_PERCENT;

  return TrafficLiftCategory::WORSE;
}

const MaskedIcon &
GetFlarmThermalIcon(const MapLook &look, double lift_rate,
                    double mac_cready) noexcept
{
  switch (ClassifyTrafficLift(lift_rate, mac_cready)) {
  case TrafficLiftCategory::BETTER:
    return look.flarm_thermal_source_green_icon;

  case TrafficLiftCategory::WITHIN_TEN_PERCENT:
    return look.flarm_thermal_source_blue_icon;

  case TrafficLiftCategory::WORSE:
    return look.flarm_thermal_source_icon;
  }

  return look.flarm_thermal_source_icon;
}

GeoPoint
GetLocation(const ThermalSource &source, double aircraft_altitude,
            const SpeedVector &wind) noexcept
{
  if (aircraft_altitude < source.ground_height)
    return GeoPoint::Invalid();

  return source.CalculateAdjustedLocation(aircraft_altitude, wind);
}

GeoPoint
GetLocation(const TrafficThermalSource &source,
            double aircraft_altitude) noexcept
{
  if (aircraft_altitude < source.thermal.ground_height)
    return GeoPoint::Invalid();

  return source.CalculateAdjustedLocation(aircraft_altitude);
}

} // namespace ThermalDisplay
