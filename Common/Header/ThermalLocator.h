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

#ifndef THERMALLOCATOR_H
#define THERMALLOCATOR_H

#define TLOCATOR_NMIN 5
#define TLOCATOR_NMAX 60

#include "Math/leastsqs.h"
#include "Thread/Mutex.hpp"
#include "GeoPoint.hpp"

class ThermalLocator_Point {
 public:
  ThermalLocator_Point() {
    valid = false;
  }
  double latitude;
  double longitude;
  double t;
  double w;
  //  double logw;
  double d;
  bool valid;
  double weight;

  void Drift(double t_0,
	     double longitude_0, double latitude_0,
	     double wind_lon, double wind_lat, double decay);
  double x;
  double y;
  int xiw;
  int yiw;
  int iweight;
  int iw;
};

class ThermalLocator {
 public:
  ThermalLocator();

  void Reset();
  void AddPoint(const double t, const GEOPOINT &location, const double w);
  void Update(const double t_0,
	      const GEOPOINT &location_0,
	      const double wind_speed, const double wind_bearing,
	      const double trackbearing,
	      GEOPOINT *Thermal_Location,
	      double *Thermal_W,
	      double *Thermal_R);

  //  double Estimate(double t_x, double t_y);

  void EstimateThermalBase(const GEOPOINT Thermal_Location,
			   const double altitude,
			   const double wthermal,
			   const double wind_speed,
			   const double wind_bearing,
                           GEOPOINT *ground_location,
			   double *ground_alt);
  double est_x;
  double est_y;
  double est_w;
  double est_r;
  double est_t;
  double est_latitude;
  double est_longitude;
 private:

  void Update_Internal(double t_0,
                       double longitude_0, double latitude_0,
                       double traildrift_lon, double traildrift_lat,
                       double trackbearing,
                       double decay,
                       double *Thermal_Longitude,
                       double *Thermal_Latitude,
                       double *Thermal_W,
                       double *Thermal_R);

  void Drift(double t_0,
	     double longitude_0, double latitude_0,
	     double wind_lon, double wind_lat, double decay);
  ThermalLocator_Point points[TLOCATOR_NMAX];
  LeastSquares ols;
  bool initialised;
  int nindex;
  int npoints;
  Mutex mutexThermalLocator;
};

#endif
