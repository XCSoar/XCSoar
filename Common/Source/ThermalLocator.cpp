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

#include "XCSoar.h"
#include "ThermalLocator.h"
#include "RasterTerrain.h"
#include "Utils.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"

#include <math.h>

int EnableThermalLocator = 1;

#define SFACT 111195

void ThermalLocator_Point::Drift(double t_0,
				 double longitude_0, double latitude_0,
				 double drift_lon, double drift_lat,
                                 double decay) {

  // convert to flat earth coordinates, then drift by wind and delta t
  double dt = t_0-t;
  weight = (exp(-1.5*decay*dt/TLOCATOR_NMAX));
  x = (longitude+drift_lon*dt-longitude_0)*fastcosine(latitude_0);
  y = (latitude+drift_lat*dt-latitude_0);

  iweight = iround(weight*100);
  xiw = iround(x*SFACT*iweight);
  yiw = iround(y*SFACT*iweight);

};


ThermalLocator::ThermalLocator() {
  initialised = true;
  Reset();
}


void ThermalLocator::Reset() {
  if (initialised) {
    initialised = false;

    // clear array
    for (int i=0; i<TLOCATOR_NMAX; i++) {
      points[i].valid = false;
    }
    nindex = 0;
    npoints = 0;
  }
}


void ThermalLocator::AddPoint(double t, double longitude, double latitude, double w) {
  points[nindex].longitude = longitude;
  points[nindex].latitude = latitude;
  points[nindex].t = t;
  points[nindex].w = w;
  points[nindex].iw = iround(max(w,-0.1)*10);
  //  points[nindex].logw = log(max(w,0.1)*10.0);
  points[nindex].valid = true;
  nindex++;
  nindex = (nindex % TLOCATOR_NMAX);

  if (npoints<TLOCATOR_NMAX-1) {
    npoints++;
  }

  if (!initialised) {
    initialised = true;

    // set initial estimate
    est_longitude = longitude;
    est_latitude = latitude;
    est_r = 0;
    est_w = 0;
    est_t = t;
  }

}

void ThermalLocator::Update(double t_0,
			    double longitude_0, double latitude_0,
			    double wind_speed, double wind_bearing,
			    double trackbearing,
			    double *Thermal_Longitude,
			    double *Thermal_Latitude,
			    double *Thermal_W,
			    double *Thermal_R) {

  if (npoints<TLOCATOR_NMIN) {
    *Thermal_R = -1;
    *Thermal_W = 0;
    return; // nothing to do.
  }

  double dlat1, dlon1;

  FindLatitudeLongitude(latitude_0,
                        longitude_0,
                        wind_bearing,
                        wind_speed, &dlat1, &dlon1);

  double traildrift_lat = (latitude_0-dlat1);
  double traildrift_lon = (longitude_0-dlon1);

  // drift estimate from previous time step
  double dt = t_0-est_t;
  est_longitude += traildrift_lon*dt;
  est_latitude += traildrift_lat*dt;
  est_x = (est_longitude-longitude_0)*fastcosine(latitude_0);
  est_y = (est_latitude-latitude_0);

  double Thermal_Longitude0;
  double Thermal_Latitude0;
  double Thermal_W0;
  double Thermal_R0;

  Update_Internal(t_0, longitude_0, latitude_0,
                  traildrift_lon, traildrift_lat,
                  trackbearing, 1.0,
                  Thermal_Longitude,
                  Thermal_Latitude,
                  Thermal_W,
                  Thermal_R);

  Update_Internal(t_0, longitude_0, latitude_0,
                  traildrift_lon, traildrift_lat,
                  trackbearing, 2.0,
                  &Thermal_Longitude0,
                  &Thermal_Latitude0,
                  &Thermal_W0,
                  &Thermal_R0);

  if ((Thermal_W0>0)&&(*Thermal_W>0)) {

    double d;
    DistanceBearing(*Thermal_Latitude, *Thermal_Longitude,
                    Thermal_Latitude0, Thermal_Longitude0,
                    &d, NULL);

    //    if (d>200.0) {
    // big shift detected

#ifdef DEBUG_THERMAL_LOCATOR
    DebugStore("%f %f %f %f %f # center2 \n",
	    *Thermal_Longitude, *Thermal_Latitude,
            Thermal_Longitude0, Thermal_Latitude0,
            d);
#endif

    //    }

  }

}



void ThermalLocator::Update_Internal(double t_0,
                                     double longitude_0,
                                     double latitude_0,
                                     double traildrift_lon,
                                     double traildrift_lat,
                                     double trackbearing,
                                     double decay,
                                     double *Thermal_Longitude,
                                     double *Thermal_Latitude,
                                     double *Thermal_W,
                                     double *Thermal_R) {

  // drift points (only do this once)
  Drift(t_0, longitude_0, latitude_0, traildrift_lon, traildrift_lat, decay);

  int slogw = 0;
  int sx=0;
  int sy=0;
  int i;

  int xav=0;
  int yav=0;

  for (i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      xav+= points[i].xiw;
      yav+= points[i].yiw;
      slogw += points[i].iweight;
    }
  }
  xav/= slogw;
  yav/= slogw;

  // xav, yav is average glider's position

  slogw = 0;
  for (i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      int dx = (points[i].xiw-xav*points[i].iweight)*points[i].iw;
      int dy = (points[i].yiw-yav*points[i].iweight)*points[i].iw;
      sx += dx;
      sy += dy;
      slogw += points[i].iw*points[i].iweight;
    }
  }
  if (slogw>0.25) {
    sx /= slogw;
    sy /= slogw;

    //    int vx = iround(100*fastsine(trackbearing));
    //    int vy = iround(100*fastcosine(trackbearing));
    //    long dx = sx;
    //    long dy = sy;
    //    int mag = isqrt4((dx*dx+dy*dy)*256*256)/256;

    // find magnitude of angle error
//    double g = max(-0.99,min(0.99,(dx*vx + dy*vy)/(100.0*mag)));
//    double angle = acos(g)*RAD_TO_DEG-90;

    est_x = (sx+xav)/(1.0*SFACT);
    est_y = (sy+yav)/(1.0*SFACT);

    est_t =  t_0;
    est_latitude = est_y+latitude_0;
    est_longitude = est_x/fastcosine(latitude_0)+longitude_0;

    *Thermal_Longitude = est_longitude;
    *Thermal_Latitude = est_latitude;
    *Thermal_R = 1;
    *Thermal_W = 1;
  } else {
    *Thermal_R = -1;
    *Thermal_W = 0;
  }
}


void ThermalLocator::Drift(double t_0,
			   double longitude_0, double latitude_0,
			   double wind_lon, double wind_lat, double decay) {

  for (int i=0; i<TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      points[i].Drift(t_0, longitude_0, latitude_0, wind_lon, wind_lat, decay);
    }
  }
}


void ThermalLocator::EstimateThermalBase(double Thermal_Longitude,
					 double Thermal_Latitude,
					 double altitude,
					 double wthermal,
					 double wind_speed,
					 double wind_bearing,
					 double *ground_longitude,
					 double *ground_latitude,
					 double *ground_alt) {

  if ((Thermal_Longitude == 0.0)||(Thermal_Latitude==0.0)||(wthermal<1.0)) {
    *ground_longitude = 0.0;
    *ground_latitude = 0.0;
    *ground_alt = -1.0;
    return;
  }

  double Tmax;
  Tmax = (altitude/wthermal);
  double dt = Tmax/10;

  RasterTerrain::Lock();

  double lat, lon;
  FindLatitudeLongitude(Thermal_Latitude, Thermal_Longitude,
                        wind_bearing,
                        wind_speed*dt,
                        &lat, &lon);
  double Xrounding = fabs(lon-Thermal_Longitude)/2;
  double Yrounding = fabs(lat-Thermal_Latitude)/2;
  RasterTerrain::SetTerrainRounding(Xrounding, Yrounding);

//  double latlast = lat;
//  double lonlast = lon;
  double hground;

  for (double t = 0; t<=Tmax; t+= dt) {

    FindLatitudeLongitude(Thermal_Latitude, Thermal_Longitude,
                          wind_bearing,
                          wind_speed*t, &lat, &lon);

    double hthermal = altitude-wthermal*t;
    hground = RasterTerrain::GetTerrainHeight(lat, lon);
    double dh = hthermal-hground;
    if (dh<0) {
      t = t+dh/wthermal;
      FindLatitudeLongitude(Thermal_Latitude, Thermal_Longitude,
                            wind_bearing,
                            wind_speed*t, &lat, &lon);
      break;
    }
  }
  hground = RasterTerrain::GetTerrainHeight(lat, lon);
  RasterTerrain::Unlock();

  *ground_longitude = lon;
  *ground_latitude = lat;
  *ground_alt = hground;

}
