/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Math/Geometry.hpp"
#include "Math/NavFunctions.hpp"

#include <assert.h>

#ifdef INSTRUMENT_TASK
unsigned count_distbearing = 0;
#endif

static const fixed fixed_earth_r = 6371000;
static const fixed fixed_double_earth_r = 6371000*2;
static const fixed fixed_inv_earth_r = (1.0/6371000);
static const fixed fixed_xtd_fact = (fixed_rad_to_deg*111194.9267);
static const fixed fixed_xte_fact = 1.0/(fixed_rad_to_deg * fixed_xtd_fact); 

#ifdef FIXED_MATH
  // need to expand range for meter accuracy
  static const fixed fixed_expand_xsq = 1e8;
  static const fixed fixed_expand_x = 1e4;
#else
#define fixed_expand_xsq 1.0
#define fixed_expand_x 1.0
#endif



GEOPOINT IntermediatePoint(GEOPOINT loc1,
                           GEOPOINT loc2,
                           fixed dthis,
                           fixed dtotal) 
{
  if ((loc1.Longitude == loc2.Longitude) && (loc1.Latitude == loc2.Latitude)){
    return loc1;
  }

  if (!positive(dtotal)) {
    return loc1;
  }
  
  const fixed f = dthis/dtotal;
  assert((f<=fixed_one) && (f>=fixed_zero));

  const fixed d = dtotal;
  const fixed inv_sind = fixed_one/sin(d);

  const fixed A=sin((fixed_one-f)*d)*inv_sind;
  const fixed B=sin(f*d)*inv_sind;

  fixed sinLoc1Latitude, cosLoc1Latitude;
  sin_cos(loc1.Latitude, &sinLoc1Latitude, &cosLoc1Latitude);
  fixed sinLoc2Latitude, cosLoc2Latitude;
  sin_cos(loc2.Latitude, &sinLoc2Latitude, &cosLoc2Latitude);

  fixed sinLoc1Longitude, cosLoc1Longitude;
  sin_cos(loc1.Longitude, &sinLoc1Longitude, &cosLoc1Longitude);
  fixed sinLoc2Longitude, cosLoc2Longitude;
  sin_cos(loc2.Longitude, &sinLoc2Longitude, &cosLoc2Longitude);

  const fixed x = A*cosLoc1Latitude*cosLoc1Longitude +  B*cosLoc2Latitude*cosLoc2Longitude;
  const fixed y = A*cosLoc1Latitude*sinLoc1Longitude +  B*cosLoc2Latitude*sinLoc2Longitude;
  const fixed z = A*sinLoc1Latitude                  +  B*sinLoc2Latitude;

  GEOPOINT loc3;
  loc3.Latitude= atan2(z,sqrt(x*x+y*y))*fixed_rad_to_deg;
  loc3.Longitude= atan2(y,x)*fixed_rad_to_deg;

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif
  return loc3;
}


GEOPOINT IntermediatePoint(GEOPOINT loc1,
                           GEOPOINT loc2,
                           const fixed dthis) 
{
  fixed ff = fixed_one/fixed_xtd_fact; // was fixed_xte_fact;
  fixed dtotal = ::Distance(loc1, loc2); 

  if (dthis>=dtotal) {
    return loc2;
  } else {
    loc1.Latitude *= fixed_deg_to_rad;
    loc2.Latitude *= fixed_deg_to_rad;
    loc1.Longitude *= fixed_deg_to_rad;
    loc2.Longitude *= fixed_deg_to_rad;
    return IntermediatePoint(loc1, loc2, dthis*ff, dtotal*ff);
  }
}

fixed CrossTrackError(GEOPOINT loc1, GEOPOINT loc2, GEOPOINT loc3,
                      GEOPOINT *loc4)
{
  fixed dist_AD, crs_AD;
  DistanceBearing(loc1, loc3, &dist_AD, &crs_AD);
  dist_AD*= fixed_xte_fact;
  crs_AD*= fixed_deg_to_rad;

  fixed dist_AB, crs_AB;
  DistanceBearing(loc1, loc2, &dist_AB, &crs_AB);
  dist_AB*= fixed_xte_fact;
  crs_AB*= fixed_deg_to_rad;

  //  The "along track distance", ATD, the distance from A along the
  //  course towards B to the point abeam D

  const fixed sindist_AD = sin(dist_AD);
  const fixed XTD = asin(sindist_AD*sin(crs_AD-crs_AB)); // cross track distance

  if (loc4) {
    fixed sinXTD, cosXTD;
    sin_cos(XTD,&sinXTD,&cosXTD);

    const fixed ATD // along track distance
      = asin(sqrt( sindist_AD*sindist_AD - sinXTD*sinXTD )/cosXTD);

    loc1.Latitude *= fixed_deg_to_rad;
    loc2.Latitude *= fixed_deg_to_rad;
    loc1.Longitude *= fixed_deg_to_rad;
    loc2.Longitude *= fixed_deg_to_rad;

    *loc4 = IntermediatePoint(loc1, loc2, ATD, dist_AB);
  }

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  // units
  return XTD*fixed_xtd_fact;
}

fixed ProjectedDistance(GEOPOINT loc1, GEOPOINT loc2, GEOPOINT loc3)
{
  fixed dist_AD, crs_AD;
  DistanceBearing(loc1, loc3, &dist_AD, &crs_AD);
  dist_AD*= fixed_xte_fact;
  crs_AD*= fixed_deg_to_rad;

  fixed dist_AB, crs_AB;
  DistanceBearing(loc1, loc2, &dist_AB, &crs_AB);
  dist_AB*= fixed_xte_fact;
  crs_AB*= fixed_deg_to_rad;

  //  The "along track distance", ATD, the distance from A along the
  //  course towards B to the point abeam D

  const fixed sindist_AD = sin(dist_AD);
  const fixed XTD = asin(sindist_AD*sin(crs_AD-crs_AB)); // cross track distance

  fixed sinXTD, cosXTD;
  sin_cos(XTD,&sinXTD,&cosXTD);

  const fixed ATD // along track distance
    = asin(sqrt( sindist_AD*sindist_AD - sinXTD*sinXTD )/cosXTD);

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return ATD*fixed_xtd_fact;
}


/**
 * Calculates the distance and bearing of two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @param Distance Pointer to the distance variable
 * @param Bearing Pointer to the bearing variable
 */
void DistanceBearing(GEOPOINT loc1, GEOPOINT loc2,
                     fixed *Distance, fixed *Bearing)
{
  loc1.Latitude *= fixed_deg_to_rad;
  loc2.Latitude *= fixed_deg_to_rad;
  loc1.Longitude *= fixed_deg_to_rad;
  loc2.Longitude *= fixed_deg_to_rad;

  fixed cloc1Latitude, sloc1Latitude;
  sin_cos(loc1.Latitude, &sloc1Latitude, &cloc1Latitude);
  fixed cloc2Latitude, sloc2Latitude;
  sin_cos(loc2.Latitude, &sloc2Latitude, &cloc2Latitude);

  const fixed dlon = loc2.Longitude-loc1.Longitude;

  if (Distance) {
    const fixed s1 = sin((loc2.Latitude-loc1.Latitude)*fixed_half)*fixed_expand_x;
    const fixed s2 = sin(dlon*fixed_half)*fixed_expand_x;
    const fixed a= max(fixed_zero, min(fixed_expand_xsq, s1*s1+cloc1Latitude*cloc2Latitude*s2*s2));

    *Distance = max(fixed_zero,fixed_double_earth_r*atan2(sqrt(a),sqrt(fixed_expand_xsq-a)));
  }
  if (Bearing) {
    fixed sindlon, cosdlon;

    // speedup for fixed since this is one call
    sin_cos(dlon, &sindlon, &cosdlon);

    const fixed y = sindlon*cloc2Latitude;
    const fixed x = cloc1Latitude*sloc2Latitude-sloc1Latitude*cloc2Latitude*cosdlon;
    *Bearing = (x==fixed_zero && y==fixed_zero) ? fixed_zero:AngleLimit360(atan2(y,x)*fixed_rad_to_deg);
  }

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif
}


fixed DoubleDistance(GEOPOINT loc1, GEOPOINT loc2, GEOPOINT loc3)
{
  loc1.Latitude *= fixed_deg_to_rad;
  loc2.Latitude *= fixed_deg_to_rad;
  loc3.Latitude *= fixed_deg_to_rad;
  loc1.Longitude *= fixed_deg_to_rad;
  loc2.Longitude *= fixed_deg_to_rad;
  loc3.Longitude *= fixed_deg_to_rad;

  const fixed cloc1Latitude = cos(loc1.Latitude);
  const fixed cloc2Latitude = cos(loc2.Latitude);
  const fixed cloc3Latitude = cos(loc3.Latitude);
  const fixed dloc2Longitude1 = loc2.Longitude-loc1.Longitude;
  const fixed dloc3Longitude2 = loc3.Longitude-loc2.Longitude;

  const fixed s21 = sin((loc2.Latitude-loc1.Latitude)*fixed_half)*fixed_expand_x;
  const fixed sl21 = sin(dloc2Longitude1*fixed_half)*fixed_expand_x;
  const fixed s32 = sin((loc3.Latitude-loc2.Latitude)*fixed_half)*fixed_expand_x;
  const fixed sl32 = sin(dloc3Longitude2*fixed_half)*fixed_expand_x;

  const fixed a12 = max(fixed_zero, min(fixed_expand_xsq,s21*s21+cloc1Latitude*cloc2Latitude*sl21*sl21));
  const fixed a23 = max(fixed_zero, min(fixed_expand_xsq,s32*s32+cloc2Latitude*cloc3Latitude*sl32*sl32));

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif

  return fixed_double_earth_r*(max(fixed_zero,atan2(sqrt(a12),sqrt(fixed_expand_xsq-a12)))
                               +max(fixed_zero,atan2(sqrt(a23),sqrt(fixed_expand_xsq-a23))));

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
void FindLatitudeLongitude(GEOPOINT loc, fixed Bearing, fixed Distance,
                           GEOPOINT *loc_out)
{
  fixed result;

  loc.Latitude *= fixed_deg_to_rad;
  loc.Longitude *= fixed_deg_to_rad;
  Bearing *= fixed_deg_to_rad;
  Distance *= fixed_inv_earth_r;

  fixed sinDistance, cosDistance;
  sin_cos(Distance, &sinDistance, &cosDistance);
  fixed sinBearing, cosBearing;
  sin_cos(Bearing, &sinBearing, &cosBearing);
  fixed sinLatitude, cosLatitude;
  sin_cos(loc.Latitude, &sinLatitude, &cosLatitude);

  assert(loc_out!= NULL); // pointless calling this otherwise

  loc_out->Latitude = (fixed)asin(sinLatitude*cosDistance
                                  +cosLatitude*sinDistance*cosBearing)*fixed_rad_to_deg;

  if (cosLatitude==fixed_zero)
    result = loc.Longitude;
  else {
    // note that asin is not supported by fixed.hpp!
    result = loc.Longitude+(fixed)asin(sinBearing*sinDistance/cosLatitude);
    result = (fixed)fmod((result+fixed_pi), fixed_two_pi)-fixed_pi;
  }
  loc_out->Longitude = result*fixed_rad_to_deg;

#ifdef INSTRUMENT_TASK
  count_distbearing++;
#endif
}

/**
 * Calculates the distance between two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @return The distance
 */
fixed Distance(GEOPOINT loc1,
               GEOPOINT loc2) {
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
fixed Bearing(GEOPOINT loc1,
              GEOPOINT loc2) {
  fixed retval;
  DistanceBearing(loc1, loc2, NULL, &retval);
  return retval;
}

