// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalDisplay.hpp"

#include "NMEA/ThermalLocator.hpp"
#include "NMEA/TrafficThermal.hpp"

namespace ThermalDisplay {

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
