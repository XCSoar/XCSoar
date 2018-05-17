/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "ThermalLocator.hpp"
#include "Geo/Math.hpp"
#include "Geo/SpeedVector.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Math/FastMath.hpp"
#include "NMEA/ThermalLocator.hpp"

#include <algorithm>

#include <assert.h>

inline void
ThermalLocator::Point::Drift(double t, const FlatProjection &projection,
                             const GeoPoint& wind_drift)
{
  const auto dt = t - t_0;

  // thermal decay function is located in GenerateSineTables.cpp
  recency_weight = thermal_recency_fn((unsigned)fabs(dt));
  lift_weight = w*recency_weight;

  GeoPoint p = location + wind_drift * dt;

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
ThermalLocator::AddPoint(const double t, const GeoPoint &location, const double w)
{
  points[n_index].location = location;
  points[n_index].t_0 = t;
  points[n_index].w = std::max(w, -0.1);
  // lift_weight and recency_weight are set by Drift()

  n_index = (n_index + 1) % TLOCATOR_NMAX;

  if (n_points < TLOCATOR_NMAX)
    n_points++;
}

void
ThermalLocator::Update(const double t_0,
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
ThermalLocator::Drift(const double t_0, const FlatProjection &projection,
                      const GeoPoint& traildrift)
{
  for (unsigned i = 0; i < n_points; ++i)
    points[i].Drift(t_0, projection, traildrift);
}

void
ThermalLocator::Process(const bool circling, const double time,
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
