/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Math.hpp"
#include "Constants.hpp"
#include "GeoPoint.hpp"

#include <assert.h>

#ifdef INSTRUMENT_TASK
// global, used for test harness
unsigned count_distbearing = 0;
#endif

#define fixed_double_earth_r fixed(REARTH * 2)

static inline Angle
EarthASin(const fixed a)
{
  return Angle::asin(a);
}

static inline Angle
EarthDistance(const fixed a)
{
  if (!positive(a))
    return Angle::Zero();

#ifdef FIXED_MATH
  // acos(1-x) = 2*asin(sqrt(x/2))
  // acos(1-2*x) = 2*asin(sqrt(x))
  //    = 2*atan2(sqrt(x), sqrt(fixed(1)-x));
  return EarthASin(sqrt(a) / (1 << fixed::accurate_cordic_shift)) * 2;
#else
  return Angle::acos(fixed(1) - Double(a));
#endif
}

gcc_pure
static GeoPoint
IntermediatePoint(const GeoPoint &loc1, const GeoPoint &loc2,
                  Angle dthis, Angle dtotal)
{
  if (loc1.longitude == loc2.longitude &&
      loc1.latitude == loc2.latitude)
    return loc1;

  if (!positive(dtotal.Native()))
    return loc1;

  assert(dthis <= dtotal && !negative(dthis.Native()));

  const fixed A = (dtotal - dthis).sin();
  const fixed B = dthis.sin();

  const auto sc1 = loc1.latitude.SinCos();
  const fixed sin_loc1_lat = sc1.first, cos_loc1_lat = sc1.second;

  const auto sc2 = loc2.latitude.SinCos();
  const fixed sin_loc2_lat = sc2.first, cos_loc2_lat = sc2.second;

  const auto sc3 = loc1.longitude.SinCos();
  const fixed sin_loc1_lon = sc3.first, cos_loc1_lon = sc3.second;

  const auto sc4 = loc2.longitude.SinCos();
  const fixed sin_loc2_lon = sc4.first, cos_loc2_lon = sc4.second;

  const fixed x = A * cos_loc1_lat * cos_loc1_lon +
                  B * cos_loc2_lat * cos_loc2_lon;
  const fixed y = A * cos_loc1_lat * sin_loc1_lon +
                  B * cos_loc2_lat * sin_loc2_lon;
  const fixed z = A * sin_loc1_lat + B * sin_loc2_lat;

  GeoPoint loc3;
  loc3.latitude = Angle::FromXY(TinyHypot(x, y), z);
  loc3.longitude = Angle::FromXY(x, y);
  loc3.Normalize(); // ensure longitude is within -180:180

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return loc3;
}

GeoPoint
IntermediatePoint(const GeoPoint &loc1, const GeoPoint &loc2,
                  const fixed dthis)
{
  const fixed dtotal = ::Distance(loc1, loc2);

  if (dthis >= dtotal)
    return loc2;

  return IntermediatePoint(loc1, loc2,
                           Angle::Radians(dthis * fixed_inv_earth_r),
                           Angle::Radians(dtotal * fixed_inv_earth_r));
}

GeoPoint
Middle(const GeoPoint &a, const GeoPoint &b)
{
  // TODO: optimize this naive approach
  const fixed distance = Distance(a, b);
  return IntermediatePoint(a, b, Half(distance));
}

/**
 * Calculates the distance and bearing of two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @param Distance Pointer to the distance variable
 * @param Bearing Pointer to the bearing variable
 */
static void
DistanceBearingS(const GeoPoint &loc1, const GeoPoint &loc2,
                 Angle *distance, Angle *bearing)
{
  const auto sc1 = loc1.latitude.SinCos();
  fixed sin_lat1 = sc1.first, cos_lat1 = sc1.second;
  const auto sc2 = loc2.latitude.SinCos();
  fixed sin_lat2 = sc2.first, cos_lat2 = sc2.second;

  const Angle dlon = loc2.longitude - loc1.longitude;

  if (distance) {
    const fixed s1 = (loc2.latitude - loc1.latitude).accurate_half_sin();
    const fixed s2 = dlon.accurate_half_sin();
    const fixed a = sqr(s1) + cos_lat1 * cos_lat2 * sqr(s2);

    Angle distance2 = EarthDistance(a);
    assert(!negative(distance2.Native()));
    *distance = distance2;
  }

  if (bearing) {
    // speedup for fixed since this is one call
    const auto sc = dlon.SinCos();
    const fixed sin_dlon = sc.first, cos_dlon = sc.second;

    const fixed y = sin_dlon * cos_lat2;
    const fixed x = cos_lat1 * sin_lat2 - sin_lat1 * cos_lat2 * cos_dlon;

    *bearing = (x == fixed(0) && y == fixed(0))
      ? Angle::Zero()
      : Angle::FromXY(x, y).AsBearing();
  }

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif
}

void
DistanceBearing(const GeoPoint &loc1, const GeoPoint &loc2,
                fixed *distance, Angle *bearing)
{
  if (distance != NULL) {
    Angle distance_angle;
    DistanceBearingS(loc1, loc2, &distance_angle, bearing);
    *distance = distance_angle.Radians() * fixed_earth_r;
  } else
    DistanceBearingS(loc1, loc2, NULL, bearing);
}

fixed
CrossTrackError(const GeoPoint &loc1, const GeoPoint &loc2,
                const GeoPoint &loc3, GeoPoint *loc4)
{
  Angle dist_AD, crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);

  Angle dist_AB, crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);

  //  The "along track distance", ATD, the distance from A along the
  //  course towards B to the point abeam D

  const fixed sindist_AD = dist_AD.sin();

  // cross track distance
  const Angle cross_track_distance =
    EarthASin(sindist_AD * (crs_AD - crs_AB).sin());

  if (loc4) {
    const auto sc = cross_track_distance.SinCos();
    const fixed sinXTD = sc.first, cosXTD = sc.second;

    const Angle along_track_distance =
      EarthASin(sqrt(sqr(sindist_AD) - sqr(sinXTD)) / cosXTD);

    *loc4 = IntermediatePoint(loc1, loc2, along_track_distance, dist_AB);
  }

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return cross_track_distance.Radians() * fixed_earth_r;
}

fixed
ProjectedDistance(const GeoPoint &loc1, const GeoPoint &loc2,
                  const GeoPoint &loc3)
{
  Angle dist_AD, crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);
  if (!positive(dist_AD.Native()))
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return fixed(0);

  Angle dist_AB, crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);
  if (!positive(dist_AB.Native()))
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return fixed(0);

  // The "along track distance", along_track_distance, the distance from A along the
  // course towards B to the point abeam D

  const fixed sindist_AD = dist_AD.sin();
  const Angle cross_track_distance =
      EarthASin(sindist_AD * (crs_AD - crs_AB).sin());

  const auto sc = cross_track_distance.SinCos();
  const fixed sinXTD = sc.first, cosXTD = sc.second;

  // along track distance
  const Angle along_track_distance =
    EarthASin(sqrt(sqr(sindist_AD) - sqr(sinXTD)) / cosXTD);

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return along_track_distance.Radians() * fixed_earth_r;
}


fixed
DoubleDistance(const GeoPoint &loc1, const GeoPoint &loc2,
               const GeoPoint &loc3)
{
  const fixed cos_loc1_lat = loc1.latitude.cos();
  const fixed cos_loc2_lat = loc2.latitude.cos();
  const fixed cos_loc3_lat = loc3.latitude.cos();

  const fixed s21 = (loc2.latitude - loc1.latitude).accurate_half_sin();
  const fixed sl21 = (loc2.longitude - loc1.longitude).accurate_half_sin();
  const fixed s32 = (loc3.latitude - loc2.latitude).accurate_half_sin();
  const fixed sl32 = (loc3.longitude - loc2.longitude).accurate_half_sin();

  const fixed a12 = sqr(s21) + cos_loc1_lat * cos_loc2_lat * sqr(sl21);
  const fixed a23 = sqr(s32) + cos_loc2_lat * cos_loc3_lat * sqr(sl32);

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return fixed_double_earth_r * 
    (EarthDistance(a12) + EarthDistance(a23)).Radians();
}

GeoPoint
FindLatitudeLongitude(const GeoPoint &loc, const Angle bearing,
                      fixed distance)
{
  assert(!negative(distance));
  if (!positive(distance))
    return loc;

  GeoPoint loc_out;
  distance /= REARTH;

  const auto scd = sin_cos(distance);
  const fixed sin_distance = scd.first, cos_distance = scd.second;

  const auto scb = bearing.SinCos();
  const fixed sin_bearing = scb.first, cos_bearing = scb.second;

  const auto scl = loc.latitude.SinCos();
  const fixed sin_latitude = scl.first, cos_latitude = scl.second;

  loc_out.latitude = EarthASin(sin_latitude * cos_distance
                               + cos_latitude * sin_distance * cos_bearing);

  loc_out.longitude = loc.longitude +
    Angle::FromXY(cos_distance - sin_latitude * loc_out.latitude.sin(),
                  sin_bearing * sin_distance * cos_latitude);

  loc_out.Normalize(); // ensure longitude is within -180:180

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return loc_out;
}

fixed
Distance(const GeoPoint &loc1, const GeoPoint &loc2)
{
  fixed distance;
  DistanceBearing(loc1, loc2, &distance, NULL);
  return distance;
}

Angle
Bearing(const GeoPoint &loc1, const GeoPoint &loc2)
{
  Angle bearing;
  DistanceBearing(loc1, loc2, NULL, &bearing);
  return bearing;
}
