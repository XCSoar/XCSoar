// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RangeBox.hpp"
#include "Geo/FAISphere.hpp"

boost::geometry::model::box<GeoPoint>
BoostRangeBox(const GeoPoint location, double range)
{
  Angle latitude_delta = FAISphere::EarthDistanceToAngle(range);

  Angle north = std::min(location.latitude + latitude_delta,
                         Angle::QuarterCircle());
  Angle south = std::max(location.latitude - latitude_delta,
                         -Angle::QuarterCircle());

  auto c = std::max(location.latitude.cos(), 0.01);
  Angle longitude_delta = std::min(latitude_delta / c, Angle::QuarterCircle());

  Angle west = (location.longitude - longitude_delta).AsDelta();
  Angle east = (location.longitude + longitude_delta).AsDelta();

  return {GeoPoint(west, south), GeoPoint(east, north)};
}
