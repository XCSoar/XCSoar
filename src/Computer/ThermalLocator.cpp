// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalLocator.hpp"
#include "ThermalRecency.hpp"
#include "Geo/Math.hpp"
#include "Geo/SpeedVector.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "NMEA/ThermalLocator.hpp"
#include "time/Cast.hxx"

#include <algorithm>

#include <cassert>

using namespace std::chrono;

inline void
ThermalLocator::Point::Drift(TimeStamp t, const FlatProjection &projection,
                             const GeoPoint& wind_drift)
{
  const auto dt = t - t_0;

  // thermal decay function is located in GenerateSineTables.cpp
  recency_weight = thermal_recency_fn(duration_cast<duration<unsigned>>(abs(dt)).count());
  lift_weight = w*recency_weight;

  GeoPoint p = location + wind_drift * ToFloatSeconds(dt);

  // convert to flat earth coordinates
  loc_drift = projection.ProjectFloat(p);
}

void
ThermalLocator::Reset()
{
  n_index = 0;
  n_points = 0;
}

inline void
ThermalLocator::AddPoint(const TimeStamp t, const GeoPoint &location,
                         const double w) noexcept
{
  points[n_index].location = location;
  points[n_index].t_0 = t;
  points[n_index].w = std::max(w, -0.1);
  // lift_weight and recency_weight are set by Drift()

  n_index = (n_index + 1) % points.size();

  if (n_points < points.size())
    n_points++;
}

void
ThermalLocator::Update(const TimeStamp t_0,
                       const GeoPoint &location_0,
                       const SpeedVector wind, 
                       ThermalLocatorInfo &therm)
{
  if (n_points < TLOCATOR_NMIN) {
    therm.estimate_valid = false;
    return; // nothing to do.
  }

  GeoPoint dloc = FindLatitudeLongitude(location_0, wind.bearing, wind.norm);

  const FlatProjection projection(location_0);

  // drift points 
  Drift(t_0, projection, location_0 - dloc);

  FlatPoint av = glider_average();
  // find thermal center relative to glider's average position

  FlatPoint f0(0, 0);
  double acc = 0;
  for (unsigned i = 0; i < n_points; ++i) {
    f0 += (points[i].loc_drift-av)*points[i].lift_weight;
    acc += points[i].lift_weight;
  }

  // if sufficient data, estimate location

  if (acc <= 0) {
    therm.estimate_valid = false;
    return;
  }
  f0 = f0 * (1. / acc) + av;

  therm.estimate_location = projection.Unproject(f0);
  therm.estimate_valid = true;
}

inline FlatPoint
ThermalLocator::glider_average()
{
  FlatPoint result(0, 0);
  assert(n_points>0);
  if (n_points == 0)
    return result;

  // find glider's average position
  double acc = 0;
  for (unsigned i = 0; i < n_points; ++i) {
    result += points[i].loc_drift*points[i].recency_weight;
    acc += points[i].recency_weight;
  }

  if (acc > 0) {
    result.x /= acc;
    result.y /= acc;
  }

  return result;
}

inline void
ThermalLocator::Drift(const TimeStamp t_0, const FlatProjection &projection,
                      const GeoPoint& traildrift)
{
  for (unsigned i = 0; i < n_points; ++i)
    points[i].Drift(t_0, projection, traildrift);
}

void
ThermalLocator::Process(const bool circling, const TimeStamp time,
                        const GeoPoint &location, const double w,
                        const SpeedVector wind, ThermalLocatorInfo& therm)
{
  if (circling) {
    AddPoint(time, location, w);
    Update(time, location, wind, therm);
  } else {
    Reset();
  }
}
