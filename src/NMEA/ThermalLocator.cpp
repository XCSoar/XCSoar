// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/ThermalLocator.hpp"
#include "NMEA/ThermalProjection.hpp"
#include "Geo/SpeedVector.hpp"
#include "util/BoundedArray.hxx"

void
ThermalLocatorInfo::Clear() noexcept
{
  estimate_valid = false;

  // clear thermal sources for first time.
  sources.clear();
}

ThermalSource &
ThermalLocatorInfo::AllocateSource() noexcept
{
  return BoundedArray::AppendOrReplaceOldest(
    sources, [](const ThermalSource &source) noexcept {
      return source.time;
    }).value;
}

GeoPoint
ThermalSource::CalculateAdjustedLocation(double altitude,
                                         const SpeedVector &wind) const noexcept
{
  return ProjectThermalCore(location, altitude - ground_height,
                            wind, lift_rate);
}
