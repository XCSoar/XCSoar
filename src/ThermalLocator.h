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

#ifndef THERMALLOCATOR_H
#define THERMALLOCATOR_H

#define TLOCATOR_NMIN 5
#define TLOCATOR_NMAX 60

#include "Math/fixed.hpp"
#include "Math/leastsqs.h"
#include "Thread/Mutex.hpp"
#include "Navigation/GeoPoint.hpp"

class ThermalLocator_Point {
 public:
  ThermalLocator_Point() {
    valid = false;
  }
  fixed latitude;
  fixed longitude;
  fixed t;
  fixed w;
  //  fixed logw;
  fixed d;
  bool valid;
  fixed weight;

  void Drift(fixed t_0,
	     fixed longitude_0, fixed latitude_0,
	     fixed wind_lon, fixed wind_lat, fixed decay);
  fixed x;
  fixed y;
  int xiw;
  int yiw;
  int iweight;
  int iw;
};

class ThermalLocator {
 public:
  ThermalLocator();

  void Reset();
  void AddPoint(const fixed t, const GEOPOINT &location, const fixed w);
  void Update(const fixed t_0,
	      const GEOPOINT &location_0,
	      const fixed wind_speed, const fixed wind_bearing,
	      const fixed trackbearing,
	      GEOPOINT *Thermal_Location,
	      fixed *Thermal_W,
	      fixed *Thermal_R);

  //  fixed Estimate(fixed t_x, fixed t_y);

  void EstimateThermalBase(const GEOPOINT Thermal_Location,
			   const fixed altitude,
			   const fixed wthermal,
			   const fixed wind_speed,
			   const fixed wind_bearing,
                           GEOPOINT *ground_location,
			   fixed *ground_alt);
  fixed est_x;
  fixed est_y;
  fixed est_w;
  fixed est_r;
  fixed est_t;
  fixed est_latitude;
  fixed est_longitude;
 private:

  void Update_Internal(fixed t_0,
                       fixed longitude_0, fixed latitude_0,
                       fixed traildrift_lon, fixed traildrift_lat,
                       fixed trackbearing,
                       fixed decay,
                       fixed *Thermal_Longitude,
                       fixed *Thermal_Latitude,
                       fixed *Thermal_W,
                       fixed *Thermal_R);

  void Drift(fixed t_0,
	     fixed longitude_0, fixed latitude_0,
	     fixed wind_lon, fixed wind_lat, fixed decay);
  ThermalLocator_Point points[TLOCATOR_NMAX];
  LeastSquares ols;
  bool initialised;
  int nindex;
  int npoints;
  Mutex mutexThermalLocator;
};

#endif
