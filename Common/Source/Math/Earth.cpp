/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include <windef.h>

#define M_2PI 6.28318530718

/**
 * Finds the point along a distance dthis between p1 and p2, which are
 * separated by dtotal.
 *
 * This is a slow function.  Adapted from The Aviation Formulary 1.42.
 */
void IntermediatePoint(GEOPOINT loc1,
                       GEOPOINT loc2,
                       double dthis,
                       double dtotal,
                       GEOPOINT *loc3) {
  double A, B, x, y, z, d, f;

  assert(loc3 != NULL);

  if ((loc1.Longitude == loc2.Longitude) && (loc1.Latitude == loc2.Latitude)){
    loc3->Latitude = loc1.Latitude;
    loc3->Longitude = loc1.Longitude;
    return;
  }

  if (dtotal>0) {
    f = dthis/dtotal;
    d = dtotal;
  } else {
    d = 1.0e-7;
    f = 0.0;
  }
  f = min(1.0,max(0.0,f));

  double cosloc1Latitude = cos(loc1.Latitude);
  double cosloc2Latitude = cos(loc2.Latitude);

  A=sin((1-f)*d)/sin(d);
  B=sin(f*d)/sin(d);
  x = A*cosloc1Latitude*cos(loc1.Longitude) +  B*cosloc2Latitude*cos(loc2.Longitude);
  y = A*cosloc1Latitude*sin(loc1.Longitude) +  B*cosloc2Latitude*sin(loc2.Longitude);
  z = A*sin(loc1.Latitude)           +  B*sin(loc2.Latitude);
  loc3->Latitude=atan2(z,sqrt(x*x+y*y))*RAD_TO_DEG;
  loc3->Longitude=atan2(y,x)*RAD_TO_DEG;
}

double CrossTrackError(GEOPOINT loc1, GEOPOINT loc2, GEOPOINT loc3,
                       GEOPOINT *loc4)
{

  double dist_AD, crs_AD;
  DistanceBearing(loc1, loc3, &dist_AD, &crs_AD);
  dist_AD/= (RAD_TO_DEG * 111194.9267); crs_AD*= DEG_TO_RAD;

  double dist_AB, crs_AB;
  DistanceBearing(loc1, loc2, &dist_AB, &crs_AB);
  dist_AB/= (RAD_TO_DEG * 111194.9267); crs_AB*= DEG_TO_RAD;

  loc1.Latitude *= DEG_TO_RAD;
  loc2.Latitude *= DEG_TO_RAD;
  loc3.Latitude *= DEG_TO_RAD;
  loc1.Longitude *= DEG_TO_RAD;
  loc2.Longitude *= DEG_TO_RAD;
  loc3.Longitude *= DEG_TO_RAD;

  double XTD; // cross track distance
  double ATD; // along track distance
  //  The "along track distance", ATD, the distance from A along the
  //  course towards B to the point abeam D

  double sindist_AD = sin(dist_AD);

  XTD = asin(sindist_AD*sin(crs_AD-crs_AB));

  double sinXTD = sin(XTD);
  ATD = asin(sqrt( sindist_AD*sindist_AD - sinXTD*sinXTD )/cos(XTD));

  if (loc4) {
    IntermediatePoint(loc1, loc2, ATD, dist_AB, loc4);
  }

  // units
  XTD *= (RAD_TO_DEG * 111194.9267);

  return XTD;
}

double ProjectedDistance(GEOPOINT loc1, GEOPOINT loc2, GEOPOINT loc3)
{
  GEOPOINT loc4;

  CrossTrackError(loc1, loc2, loc3, &loc4);
  double tmpd;
  DistanceBearing(loc1, loc4, &tmpd, NULL);
  return tmpd;
}

/**
 * Calculates the distance and bearing of two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @param Distance Pointer to the distance variable
 * @param Bearing Pointer to the bearing variable
 */
void DistanceBearing(GEOPOINT loc1, GEOPOINT loc2,
                     double *Distance, double *Bearing)
{
  loc1.Latitude *= DEG_TO_RAD;
  loc2.Latitude *= DEG_TO_RAD;
  loc1.Longitude *= DEG_TO_RAD;
  loc2.Longitude *= DEG_TO_RAD;

  double cloc1Latitude = cos(loc1.Latitude);
  double cloc2Latitude = cos(loc2.Latitude);
  double dlon = loc2.Longitude-loc1.Longitude;

  if (Distance) {
    double s1 = sin((loc2.Latitude-loc1.Latitude)/2);
    double s2 = sin(dlon/2);
    double a= max(0.0,min(1.0,s1*s1+cloc1Latitude*cloc2Latitude*s2*s2));
    *Distance = 6371000.0*2.0*atan2(sqrt(a),sqrt(1.0-a));
  }
  if (Bearing) {
    double y = sin(dlon)*cloc2Latitude;
    double x = cloc1Latitude*sin(loc2.Latitude)-sin(loc1.Latitude)*cloc2Latitude*cos(dlon);
    *Bearing = (x==0 && y==0) ? 0:AngleLimit360(atan2(y,x)*RAD_TO_DEG);
  }
}


double DoubleDistance(GEOPOINT loc1, GEOPOINT loc2, GEOPOINT loc3)
{
  loc1.Latitude *= DEG_TO_RAD;
  loc2.Latitude *= DEG_TO_RAD;
  loc3.Latitude *= DEG_TO_RAD;
  loc1.Longitude *= DEG_TO_RAD;
  loc2.Longitude *= DEG_TO_RAD;
  loc3.Longitude *= DEG_TO_RAD;

  double cloc1Latitude = cos(loc1.Latitude);
  double cloc2Latitude = cos(loc2.Latitude);
  double cloc3Latitude = cos(loc3.Latitude);
  double dloc2Longitude1 = loc2.Longitude-loc1.Longitude;
  double dloc3Longitude2 = loc3.Longitude-loc2.Longitude;

  double s21 = sin((loc2.Latitude-loc1.Latitude)/2);
  double sl21 = sin(dloc2Longitude1/2);
  double s32 = sin((loc3.Latitude-loc2.Latitude)/2);
  double sl32 = sin(dloc3Longitude2/2);

  double a12 = max(0.0,min(1.0,s21*s21+cloc1Latitude*cloc2Latitude*sl21*sl21));
  double a23 = max(0.0,min(1.0,s32*s32+cloc2Latitude*cloc3Latitude*sl32*sl32));
  return 6371000.0*2.0*(atan2(sqrt(a12),sqrt(1.0-a12))
                        +atan2(sqrt(a23),sqrt(1.0-a23)));

}

/*
double Distance(double loc1.Latitude, double loc1.Longitude, double loc2.Latitude, double loc2.Longitude)
{
    R = earth's radius = 6371000
    dlat = loc2.Latitude-loc1.Latitude;
    dlon = long2-long1
    a= sin^2(dlat/2)+cos(loc1.Latitude)*cos(loc2.Latitude)*sin^2(dlong/2)
    c= 2*atan2(sqrt(a),sqrt(1.0-a));
    d = R.c

  loc1.Latitude *= DEG_TO_RAD;
  loc2.Latitude *= DEG_TO_RAD;
  loc1.Longitude *= DEG_TO_RAD;
  loc2.Longitude *= DEG_TO_RAD;

  double dlat = loc2.Latitude-loc1.Latitude;
  double dlon = loc2.Longitude-loc1.Longitude;
  double s1 = sin(dlat/2);
  double s2 = sin(dlon/2);
  double a= s1*s1+cos(loc1.Latitude)*cos(loc2.Latitude)*s2*s2;
  double c= 2.0*atan2(sqrt(a),sqrt(1.0-a));
  return 6371000.0*c;

  // Old code... broken
  double distance, dTmp;

  dTmp =  sin(loc1.Latitude)*sin(loc2.Latitude) +
			cos(loc1.Latitude)*cos(loc2.Latitude) * cos(loc1.Longitude-loc2.Longitude);

  if (dTmp > 1.0)         // be shure we dont call acos with
    distance = 0;         // values greater than 1 (like 1.0000000000001)
  else
    distance = (double)acos(dTmp) * (double)(RAD_TO_DEG * 111194.9267);
  return (double)(distance);
}
  */

/*
double Bearing(double loc1.Latitude, double loc1.Longitude, double loc2.Latitude, double loc2.Longitude)
{
//    theta = atan2(sin(dlong)*cos(loc2.Latitude),
  //    cos(loc1.Latitude)*sin(loc2.Latitude)-sin(loc1.Latitude)*cos(loc2.Latitude)*cos(dlong));

  loc1.Latitude *= DEG_TO_RAD;
  loc2.Latitude *= DEG_TO_RAD;
  loc1.Longitude *= DEG_TO_RAD;
  loc2.Longitude *= DEG_TO_RAD;

  double cloc1.Latitude = cos(loc1.Latitude);
  double cloc2.Latitude = cos(loc2.Latitude);
  double sloc1.Latitude = sin(loc1.Latitude);
  double sloc2.Latitude = sin(loc2.Latitude);
  double dlat = loc2.Latitude-loc1.Latitude;
  double dlon = loc2.Longitude-loc1.Longitude;

  double theta =
    atan2(sin(dlon)*cloc2.Latitude,
          cloc1.Latitude*sloc2.Latitude-sloc1.Latitude*cloc2.Latitude*cos(dlon))*RAD_TO_DEG;
  while (theta>360.0) {
    theta-= 360.0;
  }
  while (theta<0.0) {
    theta+= 360.0;
  }
  return theta;

  // old code
  #ifdef HAVEEXCEPTIONS
  __try{
  #endif

  double angle;
  double d;

  d = (sloc1.Latitude*sloc2.Latitude +  cloc1.Latitude*cloc2.Latitude * cos(loc1.Longitude-loc2.Longitude) );
  if(d>1) d = 0.99999999999999;
  if(d<-1) d = -0.99999999999999;
  d = acos(d);

  if(sin(loc1.Longitude-loc2.Longitude)<0 )
    {
      angle = (((sloc2.Latitude-sloc1.Latitude)
		* cos(d) ) / (sin(d)*cloc1.Latitude));

      if(angle >1) angle = 1;
      if(angle<-1) angle = -1;
      angle = acos(angle);

      // JMW Redundant code?
      //if(loc1.Latitude>loc2.Latitude)
//	angle = angle * (180/pi);
  //    else
	//angle = angle * (180/pi);
      //
      angle *= RAD_TO_DEG;
    }
  else
    {
      if (d != 0 && cloc1.Latitude != 0){
        angle=(( (sloc2.Latitude-sloc1.Latitude)
          * cos(d) ) / (sin(d)*cloc1.Latitude));
        if(angle >1) angle = 1;
        if(angle<-1) angle = -1;
        angle = acos(angle);
      } else
        angle = 0;

      angle = 360 - (angle * RAD_TO_DEG);
    }
  #ifdef HAVEEXCEPTIONS
  }__except(EXCEPTION_EXECUTE_HANDLER ){
    return(0);
  }
  #endif

  return (double)angle;

}
*/

/**
 * Calculates the location (loc_out) you would have, after being at
 * a certain start location (loc) with a certain Bearing and going straight
 * forward for a certain Distance.
 * @param loc Current location
 * @param Bearing Current bearing
 * @param Distance Distance to predict
 * @param loc_out Future location
 */
void FindLatitudeLongitude(GEOPOINT loc, double Bearing, double Distance,
                           GEOPOINT *loc_out)
{
  double result;

  loc.Latitude *= DEG_TO_RAD;
  loc.Longitude *= DEG_TO_RAD;
  Bearing *= DEG_TO_RAD;
  Distance = Distance/6371000;

  double sinDistance = sin(Distance);
  double cosLat = cos(loc.Latitude);

  assert(loc_out!= NULL); // pointless calling this otherwise

  loc_out->Latitude = (double)asin(sin(loc.Latitude)*cos(Distance)
                                  +cosLat*sinDistance*cos(Bearing));
  loc_out->Latitude *= RAD_TO_DEG;

  if(cosLat==0)
    result = loc.Longitude;
  else {
    result = loc.Longitude+(double)asin(sin(Bearing)*sinDistance/cosLat);
    result = (double)fmod((result+M_PI),(M_2PI));
    result = result - M_PI;
  }
  result *= RAD_TO_DEG;
  loc_out->Longitude = result;
}

/**
 * Calculates the distance between two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @return The distance
 */
double Distance(GEOPOINT loc1,
                GEOPOINT loc2) {
  double retval;
  DistanceBearing(loc1, loc2, &retval, NULL);
  return retval;
};

/**
 * Calculates the bearing between two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @return The bearing
 */
double Bearing(GEOPOINT loc1,
               GEOPOINT loc2) {
  double retval;
  DistanceBearing(loc1, loc2, NULL, &retval);
  return retval;
};

