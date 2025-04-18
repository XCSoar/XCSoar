// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/ThermalLocator.hpp"
#include "Geo/SpeedVector.hpp"
#include "Geo/Math.hpp"

#include <algorithm>

void
ThermalLocatorInfo::Clear() noexcept
{
  estimate_valid = false;

  // clear thermal sources for first time.
  sources.clear();
}

static constexpr bool
CompareTime(const ThermalSource &a, const ThermalSource &b) noexcept
{
  return a.time < b.time;
}

ThermalSource &
ThermalLocatorInfo::AllocateSource() noexcept
{
  if (!sources.full())
    return sources.append();

  auto oldest = std::min_element(sources.begin(), sources.end(),
                                 CompareTime);
  assert(oldest != sources.end());
  return *oldest;
}

GeoPoint
ThermalSource::CalculateAdjustedLocation(double altitude,
                                         const SpeedVector &wind) const noexcept
{
  auto dh = altitude - ground_height;
  auto t = dh / lift_rate;
  return FindLatitudeLongitude(location, wind.bearing.Reciprocal(), wind.norm * t);
}
