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

#include "SimplifiedMath.hpp"
#include "FAISphere.hpp"
#include "GeoPoint.hpp"
#include "Math/Util.hpp"

#include <assert.h>

static inline Angle
EarthDistance(const double a)
{
  if (a <= 0)
    return Angle::Zero();

  return Angle::acos(1 - 2 * a);
}

void
DistanceBearingS(const GeoPoint &loc1, const GeoPoint &loc2,
                 Angle *distance, Angle *bearing)
{
  assert(loc1.IsValid());
  assert(loc2.IsValid());

  const auto sc1 = loc1.latitude.SinCos();
  auto sin_lat1 = sc1.first, cos_lat1 = sc1.second;
  const auto sc2 = loc2.latitude.SinCos();
  auto sin_lat2 = sc2.first, cos_lat2 = sc2.second;

  const Angle dlon = loc2.longitude - loc1.longitude;

  if (distance) {
    const auto s1 = (loc2.latitude - loc1.latitude).accurate_half_sin();
    const auto s2 = dlon.accurate_half_sin();
    const auto a = Square(s1) + cos_lat1 * cos_lat2 * Square(s2);

    Angle distance2 = EarthDistance(a);
    assert(!distance2.IsNegative());
    *distance = distance2;
  }

  if (bearing) {
    const auto sc = dlon.SinCos();
    const auto sin_dlon = sc.first, cos_dlon = sc.second;

    const auto y = sin_dlon * cos_lat2;
    const auto x = cos_lat1 * sin_lat2 - sin_lat1 * cos_lat2 * cos_dlon;

    *bearing = (x == 0 && y == 0)
      ? Angle::Zero()
      : Angle::FromXY(x, y).AsBearing();
  }
}

void
DistanceBearingS(const GeoPoint &loc1, const GeoPoint &loc2,
                 double *distance, Angle *bearing)
{
  if (distance != nullptr) {
    Angle distance_angle;
    DistanceBearingS(loc1, loc2, &distance_angle, bearing);
    *distance = FAISphere::AngleToEarthDistance(distance_angle);
  } else
    DistanceBearingS(loc1, loc2, (Angle *)nullptr, bearing);
}

GeoPoint
FindLatitudeLongitudeS(const GeoPoint &loc, const Angle bearing,
                       double distance)
{
  assert(loc.IsValid());
  assert(distance >= 0);

  if (distance <= 0)
    return loc;

  const Angle distance_angle = FAISphere::EarthDistanceToAngle(distance);

  const auto scd = distance_angle.SinCos();
  const auto sin_distance = scd.first, cos_distance = scd.second;

  const auto scb = bearing.SinCos();
  const auto sin_bearing = scb.first, cos_bearing = scb.second;

  const auto scl = loc.latitude.SinCos();
  const auto sin_latitude = scl.first, cos_latitude = scl.second;

  GeoPoint loc_out;
  loc_out.latitude = Angle::asin(sin_latitude * cos_distance
                                 + cos_latitude * sin_distance * cos_bearing);

  loc_out.longitude = loc.longitude +
    Angle::FromXY(cos_distance - sin_latitude * loc_out.latitude.sin(),
                  sin_bearing * sin_distance * cos_latitude);

  loc_out.Normalize(); // ensure longitude is within -180:180

  return loc_out;
}

double
ProjectedDistanceS(const GeoPoint &loc1, const GeoPoint &loc2,
                   const GeoPoint &loc3)
{
  Angle dist_AD, crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);
  if (!dist_AD.IsPositive())
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return 0;

  Angle dist_AB, crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);
  if (!dist_AB.IsPositive())
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return 0;

  // The "along track distance", along_track_distance, the distance from A along the
  // course towards B to the point abeam D

  const auto sindist_AD = dist_AD.sin();
  const auto cross_track_distance =
    Angle::asin(sindist_AD * (crs_AD - crs_AB).sin());

  const auto sc = cross_track_distance.SinCos();
  const auto sinXTD = sc.first, cosXTD = sc.second;

  // along track distance
  const Angle along_track_distance =
    Angle::asin(Cathetus(sindist_AD, sinXTD) / cosXTD);

  return FAISphere::AngleToEarthDistance(along_track_distance);
}
