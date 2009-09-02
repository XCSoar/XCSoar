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
void IntermediatePoint(double lon1, double lat1,
                       double lon2, double lat2,
                       double dthis,
                       double dtotal,
                       double *lon3, double *lat3) {
  double A, B, x, y, z, d, f;
  /*
  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;
  */

  assert(lat3 != NULL);
  assert(lon3 != NULL);

  if ((lon1 == lon2) && (lat1 == lat2)){
    *lat3 = lat1;
    *lon3 = lon1;
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

  double coslat1 = cos(lat1);
  double coslat2 = cos(lat2);

  A=sin((1-f)*d)/sin(d);
  B=sin(f*d)/sin(d);
  x = A*coslat1*cos(lon1) +  B*coslat2*cos(lon2);
  y = A*coslat1*sin(lon1) +  B*coslat2*sin(lon2);
  z = A*sin(lat1)           +  B*sin(lat2);
  *lat3=atan2(z,sqrt(x*x+y*y))*RAD_TO_DEG;
  *lon3=atan2(y,x)*RAD_TO_DEG;
}

double CrossTrackError(double lon1, double lat1,
                       double lon2, double lat2,
                       double lon3, double lat3,
                       double *lon4, double *lat4)
{

  double dist_AD, crs_AD;
  DistanceBearing(lat1, lon1, lat3, lon3, &dist_AD, &crs_AD);
  dist_AD/= (RAD_TO_DEG * 111194.9267); crs_AD*= DEG_TO_RAD;

  double dist_AB, crs_AB;
  DistanceBearing(lat1, lon1, lat2, lon2, &dist_AB, &crs_AB);
  dist_AB/= (RAD_TO_DEG * 111194.9267); crs_AB*= DEG_TO_RAD;

  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lat3 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;
  lon3 *= DEG_TO_RAD;

  double XTD; // cross track distance
  double ATD; // along track distance
  //  The "along track distance", ATD, the distance from A along the
  //  course towards B to the point abeam D

  double sindist_AD = sin(dist_AD);

  XTD = asin(sindist_AD*sin(crs_AD-crs_AB));

  double sinXTD = sin(XTD);
  ATD = asin(sqrt( sindist_AD*sindist_AD - sinXTD*sinXTD )/cos(XTD));

  if (lon4 && lat4) {
    IntermediatePoint(lon1, lat1, lon2, lat2, ATD, dist_AB,
                      lon4, lat4);
  }

  // units
  XTD *= (RAD_TO_DEG * 111194.9267);

  return XTD;
}

double ProjectedDistance(double lon1, double lat1,
                         double lon2, double lat2,
                         double lon3, double lat3)
{
  double lon4, lat4;

  CrossTrackError(lon1, lat1,
                  lon2, lat2,
                  lon3, lat3,
                   &lon4, &lat4);
  double tmpd;
  DistanceBearing(lat1, lon1, lat4, lon4, &tmpd, NULL);
  return tmpd;
}

void DistanceBearing(double lat1, double lon1, double lat2, double lon2,
                     double *Distance, double *Bearing)
{

  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;

  double clat1 = cos(lat1);
  double clat2 = cos(lat2);
  double dlon = lon2-lon1;

  if (Distance) {
    double s1 = sin((lat2-lat1)/2);
    double s2 = sin(dlon/2);
    double a= max(0.0,min(1.0,s1*s1+clat1*clat2*s2*s2));
    *Distance = 6371000.0*2.0*atan2(sqrt(a),sqrt(1.0-a));
  }
  if (Bearing) {
    double y = sin(dlon)*clat2;
    double x = clat1*sin(lat2)-sin(lat1)*clat2*cos(dlon);
    *Bearing = (x==0 && y==0) ? 0:AngleLimit360(atan2(y,x)*RAD_TO_DEG);
  }
}


double DoubleDistance(double lat1, double lon1, double lat2, double lon2,
                      double lat3, double lon3)
{
  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lat3 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;
  lon3 *= DEG_TO_RAD;

  double clat1 = cos(lat1);
  double clat2 = cos(lat2);
  double clat3 = cos(lat3);
  double dlon21 = lon2-lon1;
  double dlon32 = lon3-lon2;

  double s21 = sin((lat2-lat1)/2);
  double sl21 = sin(dlon21/2);
  double s32 = sin((lat3-lat2)/2);
  double sl32 = sin(dlon32/2);

  double a12 = max(0.0,min(1.0,s21*s21+clat1*clat2*sl21*sl21));
  double a23 = max(0.0,min(1.0,s32*s32+clat2*clat3*sl32*sl32));
  return 6371000.0*2.0*(atan2(sqrt(a12),sqrt(1.0-a12))
                        +atan2(sqrt(a23),sqrt(1.0-a23)));

}

/*
double Distance(double lat1, double lon1, double lat2, double lon2)
{
    R = earth's radius = 6371000
    dlat = lat2-lat1;
    dlon = long2-long1
    a= sin^2(dlat/2)+cos(lat1)*cos(lat2)*sin^2(dlong/2)
    c= 2*atan2(sqrt(a),sqrt(1.0-a));
    d = R.c

  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;

  double dlat = lat2-lat1;
  double dlon = lon2-lon1;
  double s1 = sin(dlat/2);
  double s2 = sin(dlon/2);
  double a= s1*s1+cos(lat1)*cos(lat2)*s2*s2;
  double c= 2.0*atan2(sqrt(a),sqrt(1.0-a));
  return 6371000.0*c;

  // Old code... broken
  double distance, dTmp;

  dTmp =  sin(lat1)*sin(lat2) +
			cos(lat1)*cos(lat2) * cos(lon1-lon2);

  if (dTmp > 1.0)         // be shure we dont call acos with
    distance = 0;         // values greater than 1 (like 1.0000000000001)
  else
    distance = (double)acos(dTmp) * (double)(RAD_TO_DEG * 111194.9267);
  return (double)(distance);
}
  */

/*
double Bearing(double lat1, double lon1, double lat2, double lon2)
{
//    theta = atan2(sin(dlong)*cos(lat2),
  //    cos(lat1)*sin(lat2)-sin(lat1)*cos(lat2)*cos(dlong));

  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;

  double clat1 = cos(lat1);
  double clat2 = cos(lat2);
  double slat1 = sin(lat1);
  double slat2 = sin(lat2);
  double dlat = lat2-lat1;
  double dlon = lon2-lon1;

  double theta =
    atan2(sin(dlon)*clat2,
          clat1*slat2-slat1*clat2*cos(dlon))*RAD_TO_DEG;
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

  d = (slat1*slat2 +  clat1*clat2 * cos(lon1-lon2) );
  if(d>1) d = 0.99999999999999;
  if(d<-1) d = -0.99999999999999;
  d = acos(d);

  if(sin(lon1-lon2)<0 )
    {
      angle = (((slat2-slat1)
		* cos(d) ) / (sin(d)*clat1));

      if(angle >1) angle = 1;
      if(angle<-1) angle = -1;
      angle = acos(angle);

      // JMW Redundant code?
      //if(lat1>lat2)
//	angle = angle * (180/pi);
  //    else
	//angle = angle * (180/pi);
      //
      angle *= RAD_TO_DEG;
    }
  else
    {
      if (d != 0 && clat1 != 0){
        angle=(( (slat2-slat1)
          * cos(d) ) / (sin(d)*clat1));
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

void FindLatitudeLongitude(double Lat, double Lon,
                           double Bearing, double Distance,
                           double *lat_out, double *lon_out)
{
  double result;

  Lat *= DEG_TO_RAD;
  Lon *= DEG_TO_RAD;
  Bearing *= DEG_TO_RAD;
  Distance = Distance/6371000;

  double sinDistance = sin(Distance);
  double cosLat = cos(Lat);

  if (lat_out) {
    result = (double)asin(sin(Lat)*cos(Distance)
                          +cosLat*sinDistance*cos(Bearing));
    result *= RAD_TO_DEG;
    *lat_out = result;
  }
  if (lon_out) {
    if(cosLat==0)
      result = Lon;
    else {
      result = Lon+(double)asin(sin(Bearing)*sinDistance/cosLat);
      result = (double)fmod((result+M_PI),(M_2PI));
      result = result - M_PI;
    }
    result *= RAD_TO_DEG;
    *lon_out = result;
  }
}
