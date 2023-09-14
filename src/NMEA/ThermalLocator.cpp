// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/ThermalLocator.hpp"
#include "Geo/SpeedVector.hpp"
#include "Geo/Math.hpp"

#include <algorithm>

void
ThermalLocatorInfo::Clear()
{
  estimate_valid = false;

  // clear thermal sources for first time.
  sources.clear();
}

static inline bool
CompareTime(const ThermalSource &a, const ThermalSource &b)
{
  return a.time < b.time;
}

ThermalSource &
ThermalLocatorInfo::AllocateSource()
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
                                         const SpeedVector &wind) const
{
  auto dh = altitude - ground_height;
  auto t = dh / lift_rate;
  return FindLatitudeLongitude(location, wind.bearing.Reciprocal(), wind.norm * t);
}
