/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

static inline fixed
earth_asin(const fixed a)
{
  return asin(a);
}

static inline fixed
earth_distance_function(const fixed a)
{
  if (!positive(a))
    return fixed_zero;

#ifdef FIXED_MATH
  // static const fixed fixed_shrink(fixed_two/(1<<(EXPAND_BITS*2)));
  // acos(1-x) = 2*asin(sqrt(x/2))
  // acos(1-2*x) = 2*asin(sqrt(x))
  //    = 2*atan2(sqrt(x), sqrt(fixed_one-x));
  return Double(earth_asin(sqrt(a) / (1 << fixed::accurate_cordic_shift)));
#else
  return acos(fixed_one - Double(a));
#endif
}

static GeoPoint
IntermediatePoint(const GeoPoint loc1, const GeoPoint loc2, fixed dthis,
                  fixed dtotal)
{
  if (loc1.longitude == loc2.longitude &&
      loc1.latitude == loc2.latitude)
    return loc1;

  if (!positive(dtotal))
    return loc1;

  assert(dthis <= dtotal && dthis >= fixed_zero);

  // const fixed inv_sind = fixed_one / sin(dtotal);
  // JMW remove inv_sind?

  const fixed A = sin(dtotal - dthis);// * inv_sind;
  const fixed B = sin(dthis);// * inv_sind;

  const auto sc1 = loc1.latitude.SinCos();
  const fixed sinLoc1Latitude = sc1.first, cosLoc1Latitude = sc1.second;

  const auto sc2 = loc2.latitude.SinCos();
  const fixed sinLoc2Latitude = sc2.first, cosLoc2Latitude = sc2.second;

  const auto sc3 = loc1.longitude.SinCos();
  const fixed sinLoc1Longitude = sc3.first, cosLoc1Longitude = sc3.second;

  const auto sc4 = loc2.longitude.SinCos();
  const fixed sinLoc2Longitude = sc4.first, cosLoc2Longitude = sc4.second;

  const fixed x = A * cosLoc1Latitude * cosLoc1Longitude +
                  B * cosLoc2Latitude * cosLoc2Longitude;
  const fixed y = A * cosLoc1Latitude * sinLoc1Longitude +
                  B * cosLoc2Latitude * sinLoc2Longitude;
  const fixed z = A * sinLoc1Latitude + B * sinLoc2Latitude;

  GeoPoint loc3;
  loc3.latitude = Angle::Radians(atan2(z, TinyHypot(x, y)));
  loc3.longitude = Angle::Radians(atan2(y, x));
  loc3.Normalize(); // ensure longitude is within -180:180

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return loc3;
}

GeoPoint
IntermediatePoint(const GeoPoint loc1, const GeoPoint loc2, const fixed dthis)
{
  const fixed dtotal = ::Distance(loc1, loc2);

  if (dthis >= dtotal)
    return loc2;

  return IntermediatePoint(loc1, loc2, dthis * fixed_inv_earth_r,
                           dtotal * fixed_inv_earth_r);
}

GeoPoint
Middle(GeoPoint a, GeoPoint b)
{
  // TODO: optimize this naive approach
  const fixed distance = Distance(a, b);
  return IntermediatePoint(a, b, half(distance));
}

/**
 * Calculates the distance and bearing of two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @param Distance Pointer to the distance variable
 * @param Bearing Pointer to the bearing variable
 */
static void
DistanceBearingS(const GeoPoint loc1, const GeoPoint loc2,
                 Angle *distance, Angle *bearing)
{
  const auto sc1 = loc1.latitude.SinCos();
  fixed sin_lat1 = sc1.first, cos_lat1 = sc1.second;
  const auto sc2 = loc2.latitude.SinCos();
  fixed sin_lat2 = sc2.first, cos_lat2 = sc2.second;

  const fixed dlon = (loc2.longitude - loc1.longitude).Radians();

  if (distance) {
    const fixed s1 = (loc2.latitude - loc1.latitude).accurate_half_sin();
    const fixed s2 = accurate_half_sin(dlon);
    const fixed a = sqr(s1) + cos_lat1 * cos_lat2 * sqr(s2);

    fixed distance2 = earth_distance_function(a);
    assert(!negative(distance2));
    *distance = Angle::Radians(distance2);
  }

  if (bearing) {
    // speedup for fixed since this is one call
    const auto sc = sin_cos(dlon);
    const fixed sin_dlon = sc.first, cos_dlon = sc.second;

    const fixed y = sin_dlon * cos_lat2;
    const fixed x = cos_lat1 * sin_lat2 - sin_lat1 * cos_lat2 * cos_dlon;

    *bearing = (x == fixed_zero && y == fixed_zero)
      ? Angle::Zero()
      : Angle::Radians(atan2(y, x)).AsBearing();
  }

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif
}

void
DistanceBearing(const GeoPoint loc1, const GeoPoint loc2,
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
CrossTrackError(const GeoPoint loc1, const GeoPoint loc2,
                const GeoPoint loc3, GeoPoint *loc4)
{
  Angle dist_AD, crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);

  Angle dist_AB, crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);

  //  The "along track distance", ATD, the distance from A along the
  //  course towards B to the point abeam D

  const fixed sindist_AD = dist_AD.sin();

  // cross track distance
  const fixed cross_track_distance =
    earth_asin(sindist_AD * (crs_AD - crs_AB).sin());

  if (loc4) {
    const auto sc = sin_cos(cross_track_distance);
    const fixed sinXTD = sc.first, cosXTD = sc.second;

    const fixed along_track_distance =
      earth_asin(sqrt(sqr(sindist_AD) - sqr(sinXTD)) / cosXTD);

    *loc4 = IntermediatePoint(loc1, loc2, along_track_distance, dist_AB.Radians());
  }

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return cross_track_distance * fixed_earth_r;
}

fixed
ProjectedDistance(const GeoPoint loc1, const GeoPoint loc2, const GeoPoint loc3)
{
  Angle dist_AD, crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);
  if (!positive(dist_AD.Native()))
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return fixed_zero;

  Angle dist_AB, crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);
  if (!positive(dist_AB.Native()))
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return fixed_zero;

  // The "along track distance", along_track_distance, the distance from A along the
  // course towards B to the point abeam D

  const fixed sindist_AD = dist_AD.sin();
  const fixed cross_track_distance =
      earth_asin(sindist_AD * (crs_AD - crs_AB).sin());

  const auto sc = sin_cos(cross_track_distance);
  const fixed sinXTD = sc.first, cosXTD = sc.second;

  // along track distance
  const fixed along_track_distance =
    earth_asin(sqrt(sqr(sindist_AD) - sqr(sinXTD)) / cosXTD);

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return along_track_distance * fixed_earth_r;
}


fixed
DoubleDistance(const GeoPoint loc1, const GeoPoint loc2, const GeoPoint loc3)
{
  const fixed cloc1Latitude = loc1.latitude.cos();
  const fixed cloc2Latitude = loc2.latitude.cos();
  const fixed cloc3Latitude = loc3.latitude.cos();

  const fixed s21 = (loc2.latitude - loc1.latitude).accurate_half_sin();
  const fixed sl21 = (loc2.longitude - loc1.longitude).accurate_half_sin();
  const fixed s32 = (loc3.latitude - loc2.latitude).accurate_half_sin();
  const fixed sl32 = (loc3.longitude - loc2.longitude).accurate_half_sin();

  const fixed a12 = sqr(s21) + cloc1Latitude * cloc2Latitude * sqr(sl21);
  const fixed a23 = sqr(s32) + cloc2Latitude * cloc3Latitude * sqr(sl32);

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return fixed_double_earth_r * 
    (earth_distance_function(a12) + earth_distance_function(a23));
}

GeoPoint
FindLatitudeLongitude(const GeoPoint loc, const Angle bearing,
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

  loc_out.latitude = Angle::Radians(earth_asin(
      sin_latitude * cos_distance + cos_latitude * sin_distance * cos_bearing));

  loc_out.longitude =
      Angle::Radians(loc.longitude.Radians() +
                     atan2(sin_bearing * sin_distance * cos_latitude,
                           cos_distance - sin_latitude * loc_out.latitude.sin()));

  loc_out.Normalize(); // ensure longitude is within -180:180

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return loc_out;
}

fixed
Distance(const GeoPoint loc1, const GeoPoint loc2)
{
  fixed distance;
  DistanceBearing(loc1, loc2, &distance, NULL);
  return distance;
}

Angle
Bearing(const GeoPoint loc1, const GeoPoint loc2)
{
  Angle bearing;
  DistanceBearing(loc1, loc2, NULL, &bearing);
  return bearing;
}
