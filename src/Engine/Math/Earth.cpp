/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Math/Earth.hpp"
#include <assert.h>

// global, used for test harness
unsigned count_distbearing = 0;

#define fixed_double_earth_r fixed(REARTH * 2)

static inline
fixed earth_asin(const fixed a) {
  return asin(a);
}

static inline
fixed earth_distance_function(const fixed a) {
  if (!positive(a))
    return fixed_zero;

#ifdef FIXED_MATH
  // static const fixed fixed_shrink(fixed_two/(1<<(EXPAND_BITS*2)));
  // acos(1-x) = 2*asin(sqrt(x/2))
  // acos(1-2*x) = 2*asin(sqrt(x))
  //    = 2*atan2(sqrt(x), sqrt(fixed_one-x));
  return fixed_two*earth_asin(sqrt(a)/(1<<fixed::accurate_cordic_shift));
#else
  return acos(fixed_one-fixed_two*a);
#endif
}

static GeoPoint
IntermediatePoint(const GeoPoint loc1, const GeoPoint loc2, fixed dthis, fixed dtotal)
{
  if ((loc1.longitude == loc2.longitude) && 
      (loc1.latitude == loc2.latitude))
    return loc1;

  if (!positive(dtotal))
    return loc1;

  assert((dthis <= dtotal) && (dthis >= fixed_zero));

  // const fixed inv_sind = fixed_one / sin(dtotal);
  // JMW remove inv_sind?

  const fixed A = sin(dtotal-dthis);// * inv_sind;
  const fixed B = sin(dthis);// * inv_sind;

  fixed sinLoc1Latitude, cosLoc1Latitude;
  loc1.latitude.sin_cos(sinLoc1Latitude, cosLoc1Latitude);
  fixed sinLoc2Latitude, cosLoc2Latitude;
  loc2.latitude.sin_cos(sinLoc2Latitude, cosLoc2Latitude);

  fixed sinLoc1Longitude, cosLoc1Longitude;
  loc1.longitude.sin_cos(sinLoc1Longitude, cosLoc1Longitude);
  fixed sinLoc2Longitude, cosLoc2Longitude;
  loc2.longitude.sin_cos(sinLoc2Longitude, cosLoc2Longitude);

  const fixed x = A * cosLoc1Latitude * cosLoc1Longitude
      + B * cosLoc2Latitude * cosLoc2Longitude;
  const fixed y = A * cosLoc1Latitude * sinLoc1Longitude
      + B * cosLoc2Latitude * sinLoc2Longitude;
  const fixed z = A * sinLoc1Latitude + B * sinLoc2Latitude;

  GeoPoint loc3;
  loc3.latitude = Angle::radians(atan2(z, hypot(x, y)));
  loc3.longitude = Angle::radians(atan2(y, x));
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

  if (dthis >= dtotal) {
    return loc2;
  } else {
    return IntermediatePoint(loc1, loc2, 
                             dthis*fixed_inv_earth_r, dtotal*fixed_inv_earth_r);
  }
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
                 Angle *Distance, Angle *Bearing)
{
  fixed cos_lat1, sin_lat1;
  loc1.latitude.sin_cos(sin_lat1, cos_lat1);
  fixed cos_lat2, sin_lat2;
  loc2.latitude.sin_cos(sin_lat2, cos_lat2);

  const fixed dlon = (loc2.longitude - loc1.longitude).value_radians();

  if (Distance) {
    const fixed s1 = (loc2.latitude - loc1.latitude).accurate_half_sin();
    const fixed s2 = accurate_half_sin(dlon);
    const fixed a = sqr(s1) + cos_lat1 * cos_lat2 * sqr(s2);

    fixed distance2 = earth_distance_function(a);
    assert(!negative(distance2));
    *Distance = Angle::radians(distance2);
  }

  if (Bearing) {
    fixed sin_dlon, cos_dlon;

    // speedup for fixed since this is one call
    sin_cos(dlon, &sin_dlon, &cos_dlon);

    const fixed y = sin_dlon * cos_lat2;
    const fixed x = cos_lat1 * sin_lat2 - sin_lat1 * cos_lat2 * cos_dlon;

    *Bearing = (x == fixed_zero && y == fixed_zero)
      ? Angle::zero()
      : Angle::radians(atan2(y, x)).as_bearing();
  }

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif
}


void
DistanceBearing(const GeoPoint loc1, const GeoPoint loc2,
                fixed *Distance, Angle *Bearing)
{
  if (Distance != NULL) {
    Angle distance_angle;
    DistanceBearingS(loc1, loc2, &distance_angle, Bearing);
    *Distance = distance_angle.value_radians() * fixed_earth_r;
  } else
    DistanceBearingS(loc1, loc2, NULL, Bearing);
}

fixed
CrossTrackError(const GeoPoint loc1, const GeoPoint loc2,
                const GeoPoint loc3, GeoPoint *loc4)
{
  Angle dist_AD; Angle crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);

  Angle dist_AB; Angle crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);

  //  The "along track distance", ATD, the distance from A along the
  //  course towards B to the point abeam D

  const fixed sindist_AD = dist_AD.sin();
  // cross track distance
  const fixed XTD(earth_asin(sindist_AD * (crs_AD - crs_AB).sin()));

  if (loc4) {
    fixed sinXTD, cosXTD;
    sin_cos(XTD, &sinXTD, &cosXTD);

    // along track distance
    const fixed ATD(earth_asin(sqrt(sindist_AD * sindist_AD - sinXTD * sinXTD)
                         / cosXTD));

    *loc4 = IntermediatePoint(loc1, loc2, ATD, dist_AB.value_radians());
  }

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  // units
  return XTD * fixed_earth_r;
}

fixed
ProjectedDistance(const GeoPoint loc1, const GeoPoint loc2, const GeoPoint loc3)
{
  Angle dist_AD; Angle crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);
  if (!positive(dist_AD.value_native()))
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return fixed_zero;

  Angle dist_AB; Angle crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);
  if (!positive(dist_AB.value_native()))
    /* workaround: new sine implementation may return small non-zero
       values for sin(0) */
    return fixed_zero;

  // The "along track distance", ATD, the distance from A along the
  // course towards B to the point abeam D

  const fixed sindist_AD = dist_AD.sin();
  const fixed XTD(earth_asin(sindist_AD * (crs_AD - crs_AB).sin())); // cross track distance

  fixed sinXTD, cosXTD;
  sin_cos(XTD, &sinXTD, &cosXTD);

  // along track distance
  const fixed ATD(earth_asin(sqrt(sindist_AD * sindist_AD - sinXTD * sinXTD) / cosXTD));

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return ATD * fixed_earth_r;
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

/**
 * Calculates the location (loc_out) you would have, after being at
 * a certain start location (loc) with a certain Bearing and going straight
 * forward for a certain Distance.
 * @param loc Current location
 * @param Bearing Current bearing
 * @param Distance Distance to predict
 * @param loc_out Future location
 */
GeoPoint
FindLatitudeLongitude(const GeoPoint loc, const Angle Bearing,
                      fixed Distance)
{
  assert(!negative(Distance));
  if (!positive(Distance))
    return loc;

  GeoPoint loc_out;
  Distance *= fixed_inv_earth_r;

  fixed sinDistance, cosDistance;
  sin_cos(Distance, &sinDistance, &cosDistance);

  fixed sinBearing, cosBearing;
  Bearing.sin_cos(sinBearing, cosBearing);

  fixed sinLatitude, cosLatitude;
  loc.latitude.sin_cos(sinLatitude, cosLatitude);

  loc_out.latitude = Angle::radians(earth_asin(sinLatitude * cosDistance + cosLatitude
                                         * sinDistance * cosBearing));

  fixed result;

  if (cosLatitude == fixed_zero)
    result = loc.longitude.value_radians();
  else {
    result = loc.longitude.value_radians() + 
      earth_asin(sinBearing * sinDistance / cosLatitude);
  }

  loc_out.longitude = Angle::radians(result);
  loc_out.Normalize(); // ensure longitude is within -180:180

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return loc_out;
}

/**
 * Calculates the distance between two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @return The distance
 */
fixed
Distance(const GeoPoint loc1, const GeoPoint loc2)
{
  fixed retval;
  DistanceBearing(loc1, loc2, &retval, NULL);
  return retval;
}

/**
 * Calculates the bearing between two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @return The bearing
 */
Angle
Bearing(const GeoPoint loc1, const GeoPoint loc2)
{
  Angle retval;
  DistanceBearing(loc1, loc2, NULL, &retval);
  return retval;
}
