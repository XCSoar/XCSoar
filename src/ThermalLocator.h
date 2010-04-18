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
#include "Navigation/GeoPoint.hpp"
#include "Navigation/SpeedVector.hpp"
#include "NMEA/Derived.hpp"

class ThermalLocator {
private:

  struct ThermalLocator_Point 
  {
    ThermalLocator_Point()
    {
      valid = false;
    }
    
    GEOPOINT location;          /**< Actual location of sample */
    fixed t;                    /**< Time of sample (s) */
    // fixed logw;

    bool valid;                 /**< Whether this point is valid */
    
    void Drift(fixed t_0, 
               const GEOPOINT& location_0,
               const GEOPOINT& wind_drift,
               fixed decay);
    
    int x_weighted;             /**< X coordinate of relative weighted location */
    int y_weighted;             /**< Y coordinate of relative weighted location */
    int weight;
    int w_scaled;
  };

public:
  ThermalLocator();

  void Reset();

  void Process(const bool circling,
               const fixed time, 
               const GEOPOINT &location, 
               const fixed w,
               const SpeedVector wind,
               THERMAL_LOCATOR_INFO& therm);

  void EstimateThermalBase(const GEOPOINT Thermal_Location,
                           const fixed altitude,
                           const fixed wthermal,
                           const SpeedVector wind,
                           GEOPOINT *ground_location,
                           fixed *ground_alt);

private:

  void invalid_estimate(THERMAL_LOCATOR_INFO &therm);

  fixed est_x;
  fixed est_y;
  fixed est_w;
  fixed est_r;
  fixed est_t;
  GEOPOINT est_location;

  void AddPoint(const fixed t, const GEOPOINT &location, const fixed w);
  void Update(const fixed t_0,
              const GEOPOINT &location_0,
              const SpeedVector wind,
              THERMAL_LOCATOR_INFO &therm);

  void Update_Internal(fixed t_0,
                       const GEOPOINT& location_0, 
                       const GEOPOINT& traildrift,
                       fixed decay,
                       THERMAL_LOCATOR_INFO& therm);

  void Drift(fixed t_0,
             const GEOPOINT& location_0, 
             const GEOPOINT& traildrift,
             fixed decay);

  ThermalLocator_Point points[TLOCATOR_NMAX]; /**< Circular buffer of points */

  LeastSquares ols;
  bool initialised;
  int n_index; /**< Index of next point to add */
  int n_points; /**< Number of points in buffer */
};

#endif
