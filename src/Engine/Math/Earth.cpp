/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#define REARTH 6371000
const fixed fixed_earth_r(REARTH);
const fixed fixed_inv_earth_r(1.0 / REARTH);
static const fixed fixed_double_earth_r(REARTH * 2);

#ifdef FIXED_MATH
  // need to expand range for meter accuracy
  static const fixed fixed_expand_xsq(1e8);
  static const fixed fixed_expand_x(1e4);
#else
#define fixed_expand_xsq 1.0
#define fixed_expand_x 1.0
#endif

static GeoPoint
IntermediatePoint(GeoPoint loc1, GeoPoint loc2, fixed dthis, fixed dtotal)
{
  if ((loc1.Longitude == loc2.Longitude) && 
      (loc1.Latitude == loc2.Latitude))
    return loc1;

  if (!positive(dtotal))
    return loc1;

  const fixed f = dthis / dtotal;
  assert((f <= fixed_one) && (f >= fixed_zero));

  const fixed d = dtotal;
  const fixed inv_sind = fixed_one / sin(d);

  const fixed A = sin((fixed_one - f) * d) * inv_sind;
  const fixed B = sin(f * d) * inv_sind;

  fixed sinLoc1Latitude, cosLoc1Latitude;
  loc1.Latitude.sin_cos(sinLoc1Latitude, cosLoc1Latitude);
  fixed sinLoc2Latitude, cosLoc2Latitude;
  loc2.Latitude.sin_cos(sinLoc2Latitude, cosLoc2Latitude);

  fixed sinLoc1Longitude, cosLoc1Longitude;
  loc1.Longitude.sin_cos(sinLoc1Longitude, cosLoc1Longitude);
  fixed sinLoc2Longitude, cosLoc2Longitude;
  loc2.Longitude.sin_cos(sinLoc2Longitude, cosLoc2Longitude);

  const fixed x = A * cosLoc1Latitude * cosLoc1Longitude
      + B * cosLoc2Latitude * cosLoc2Longitude;
  const fixed y = A * cosLoc1Latitude * sinLoc1Longitude
      + B * cosLoc2Latitude * sinLoc2Longitude;
  const fixed z = A * sinLoc1Latitude + B * sinLoc2Latitude;

  GeoPoint loc3;
  loc3.Latitude = Angle::radians(atan2(z, hypot(x, y)));
  loc3.Longitude = Angle::radians(atan2(y, x));

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return loc3;
}

GeoPoint
IntermediatePoint(GeoPoint loc1, GeoPoint loc2, const fixed dthis)
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
DistanceBearingS(GeoPoint loc1, GeoPoint loc2, Angle *Distance, Angle *Bearing)
{
  fixed cos_lat1, sin_lat1;
  loc1.Latitude.sin_cos(sin_lat1, cos_lat1);
  fixed cos_lat2, sin_lat2;
  loc2.Latitude.sin_cos(sin_lat2, cos_lat2);

  const fixed dlon = (loc2.Longitude - loc1.Longitude).value_radians();

  if (Distance) {
    const fixed s1 = ((loc2.Latitude - loc1.Latitude) * fixed_half).sin()
        * fixed_expand_x;
    const fixed s2 = sin(dlon / 2) * fixed_expand_x;
    const fixed a = max(fixed_zero, 
                        min(fixed_expand_xsq,
                            s1 * s1 + cos_lat1 * cos_lat2 * s2 * s2));

    fixed distance2 = max(fixed_zero, 2 * atan2(sqrt(a), sqrt(fixed_expand_xsq - a)));
    *Distance = Angle::radians(distance2);
  }

  if (Bearing) {
    fixed sin_dlon, cos_dlon;

    // speedup for fixed since this is one call
    sin_cos(dlon, &sin_dlon, &cos_dlon);

    const fixed y = sin_dlon * cos_lat2;
    const fixed x = cos_lat1 * sin_lat2 - sin_lat1 * cos_lat2 * cos_dlon;

    *Bearing = (x == fixed_zero && y == fixed_zero)
      ? Angle::native(fixed_zero)
      : Angle::radians(atan2(y, x)).as_bearing();
  }

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif
}


void
DistanceBearing(GeoPoint loc1, GeoPoint loc2, fixed *Distance, Angle *Bearing)
{
  if (Distance != NULL) {
    Angle distance_angle;
    DistanceBearingS(loc1, loc2, &distance_angle, Bearing);
    *Distance = distance_angle.value_radians() * fixed_earth_r;
  } else
    DistanceBearingS(loc1, loc2, NULL, Bearing);
}

fixed
CrossTrackError(GeoPoint loc1, GeoPoint loc2, GeoPoint loc3, GeoPoint *loc4)
{
  Angle dist_AD; Angle crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);

  Angle dist_AB; Angle crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);

  //  The "along track distance", ATD, the distance from A along the
  //  course towards B to the point abeam D

  const fixed sindist_AD = dist_AD.sin();
  // cross track distance
  const fixed XTD(asin(sindist_AD * (crs_AD - crs_AB).sin()));

  if (loc4) {
    fixed sinXTD, cosXTD;
    sin_cos(XTD, &sinXTD, &cosXTD);

    // along track distance
    const fixed ATD(asin(sqrt(sindist_AD * sindist_AD - sinXTD * sinXTD)
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
ProjectedDistance(GeoPoint loc1, GeoPoint loc2, GeoPoint loc3)
{
  Angle dist_AD; Angle crs_AD;
  DistanceBearingS(loc1, loc3, &dist_AD, &crs_AD);

  Angle dist_AB; Angle crs_AB;
  DistanceBearingS(loc1, loc2, &dist_AB, &crs_AB);

  // The "along track distance", ATD, the distance from A along the
  // course towards B to the point abeam D

  const fixed sindist_AD = dist_AD.sin();
  const fixed XTD(asin(sindist_AD * (crs_AD - crs_AB).sin())); // cross track distance

  fixed sinXTD, cosXTD;
  sin_cos(XTD, &sinXTD, &cosXTD);

  // along track distance
  const fixed ATD(asin(sqrt(sindist_AD * sindist_AD - sinXTD * sinXTD) / cosXTD));

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return ATD * fixed_earth_r;
}


fixed
DoubleDistance(GeoPoint loc1, GeoPoint loc2, GeoPoint loc3)
{
  const fixed cloc1Latitude = loc1.Latitude.cos();
  const fixed cloc2Latitude = loc2.Latitude.cos();
  const fixed cloc3Latitude = loc3.Latitude.cos();
  const fixed dloc2Longitude1 = (loc2.Longitude - loc1.Longitude).value_radians();
  const fixed dloc3Longitude2 = (loc3.Longitude - loc2.Longitude).value_radians();

  const fixed s21 = ((loc2.Latitude - loc1.Latitude) * fixed_half).sin()
      * fixed_expand_x;

  const fixed sl21 = sin(dloc2Longitude1 * fixed_half)
      * fixed_expand_x;

  const fixed s32 = ((loc3.Latitude - loc2.Latitude) * fixed_half).sin()
      * fixed_expand_x;

  const fixed sl32 = sin(dloc3Longitude2 * fixed_half)
      * fixed_expand_x;

  const fixed a12 = max(fixed_zero, min(fixed_expand_xsq, s21 * s21
      + cloc1Latitude * cloc2Latitude * sl21 * sl21));

  const fixed a23 = max(fixed_zero, min(fixed_expand_xsq, s32 * s32
      + cloc2Latitude * cloc3Latitude * sl32 * sl32));

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return fixed_double_earth_r * (max(fixed_zero, atan2(sqrt(a12), sqrt(
      fixed_expand_xsq - a12))) + max(fixed_zero, atan2(sqrt(a23), sqrt(
      fixed_expand_xsq - a23))));
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
FindLatitudeLongitude(GeoPoint loc, Angle Bearing, 
                      fixed Distance)
{
  if (!positive(Distance))
    return loc;

  GeoPoint loc_out;
  Distance *= fixed_inv_earth_r;

  fixed sinDistance, cosDistance;
  sin_cos(Distance, &sinDistance, &cosDistance);

  fixed sinBearing, cosBearing;
  Bearing.sin_cos(sinBearing, cosBearing);

  fixed sinLatitude, cosLatitude;
  loc.Latitude.sin_cos(sinLatitude, cosLatitude);

  loc_out.Latitude = Angle::radians((fixed)asin(sinLatitude * cosDistance + cosLatitude
                                          * sinDistance * cosBearing));

  fixed result;

  if (cosLatitude == fixed_zero)
    result = loc.Longitude.value_radians();
  else {
    // note that asin is not supported by fixed.hpp!
    result = loc.Longitude.value_radians() + 
      asin(sinBearing * sinDistance / cosLatitude);
    result = fmod((result + fixed_pi), fixed_two_pi) - fixed_pi;
  }

  loc_out.Longitude = Angle::radians(result);

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
Distance(GeoPoint loc1, GeoPoint loc2)
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
Bearing(GeoPoint loc1, GeoPoint loc2)
{
  Angle retval;
  DistanceBearing(loc1, loc2, NULL, &retval);
  return retval;
}
