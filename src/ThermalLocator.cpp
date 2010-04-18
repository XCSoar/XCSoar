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

#include "ThermalLocator.h"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Components.hpp"
#include <math.h>


#define SFACT 111195

void
ThermalLocator_Point::Drift(fixed t_0, 
                            const GEOPOINT& location_0,
                            const GEOPOINT& wind_drift,
                            fixed decay)
{
  // convert to flat earth coordinates, then drift by wind and delta t
  fixed dt = t_0 - t;

  weight = (exp(-1.5 * decay * dt / TLOCATOR_NMAX));

  x = (location.Longitude + wind_drift.Longitude * dt - location_0.Longitude) 
    * fastcosine(location_0.Latitude);
  y = (location.Latitude + wind_drift.Latitude * dt - location_0.Latitude);

  iweight = iround(weight * 100);
  xiw = iround(x * SFACT * iweight);
  yiw = iround(y * SFACT * iweight);
}

ThermalLocator::ThermalLocator()
{
  initialised = true;
  Reset();
}

void
ThermalLocator::Reset()
{
  if (initialised) {
    initialised = false;

    // clear array
    for (int i = 0; i < TLOCATOR_NMAX; i++) {
      points[i].valid = false;
    }
    nindex = 0;
    npoints = 0;
  }
}

void
ThermalLocator::AddPoint(const fixed t, const GEOPOINT &location, const fixed w)
{
  points[nindex].location = location;
  points[nindex].t = t;
  points[nindex].w = w;
  points[nindex].iw = iround(max(w, fixed(-0.1)) * 10);
  // points[nindex].logw = log(max(w,0.1)*10.0);
  points[nindex].valid = true;

  nindex++;
  nindex = (nindex % TLOCATOR_NMAX);

  if (npoints < TLOCATOR_NMAX - 1)
    npoints++;

  if (!initialised) {
    initialised = true;

    // set initial estimate
    est_location = location;
    est_r = 0;
    est_w = 0;
    est_t = t;
  }
}

void
ThermalLocator::invalid_estimate(THERMAL_LOCATOR_INFO &therm)
{
  therm.ThermalEstimate_R = -1;
  therm.ThermalEstimate_W = 0;
}

void
ThermalLocator::Update(const fixed t_0, const GEOPOINT &location_0,
                       const SpeedVector wind, 
                       THERMAL_LOCATOR_INFO &therm)
{
  if (npoints < TLOCATOR_NMIN) {
    invalid_estimate(therm);
    return; // nothing to do.
  }

  GEOPOINT dloc;

  FindLatitudeLongitude(location_0, wind.bearing, wind.norm, &dloc);

  GEOPOINT traildrift = location_0-dloc;

  // drift estimate from previous time step
  fixed dt = t_0 - est_t;
  est_location += traildrift * dt;
  est_x = (est_location.Longitude - location_0.Longitude) * fastcosine(location_0.Latitude);
  est_y = (est_location.Latitude - location_0.Latitude);

  Update_Internal(t_0, 
                  location_0, 
                  traildrift, 
                  fixed_one, therm);
  /*
  THERMAL_LOCATION_INFO therm0;

  Update_Internal(t_0, location_0,
                  traildrift, 
                  fixed_two, therm0);
  */
}

void
ThermalLocator::Update_Internal(fixed t_0, 
                                const GEOPOINT& location_0, 
                                const GEOPOINT& traildrift,
                                fixed decay, 
                                THERMAL_LOCATOR_INFO &therm)
{
  // drift points (only do this once)
  Drift(t_0, location_0, traildrift, decay);

  int slogw = 0;
  int sx = 0;
  int sy = 0;
  int i;

  int xav = 0;
  int yav = 0;

  for (i = 0; i < TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      xav += points[i].xiw;
      yav += points[i].yiw;
      slogw += points[i].iweight;
    }
  }
  xav /= slogw;
  yav /= slogw;

  // xav, yav is average glider's position

  slogw = 0;
  for (i = 0; i < TLOCATOR_NMAX; i++) {
    if (points[i].valid) {
      int dx = (points[i].xiw - xav * points[i].iweight) * points[i].iw;
      int dy = (points[i].yiw - yav * points[i].iweight) * points[i].iw;
      sx += dx;
      sy += dy;
      slogw += points[i].iw * points[i].iweight;
    }
  }

  if (slogw > 0.25) {
    sx /= slogw;
    sy /= slogw;

    est_x = (sx + xav) / (1.0 * SFACT);
    est_y = (sy + yav) / (1.0 * SFACT);

    est_t = t_0;
    est_location.Latitude = est_y + location_0.Latitude;
    est_location.Longitude = est_x / fastcosine(location_0.Latitude) + location_0.Longitude;

    therm.ThermalEstimate_Location = est_location;
    therm.ThermalEstimate_R = 1;
    therm.ThermalEstimate_W = 1;
  } else {
    invalid_estimate(therm);
  }
}

void
ThermalLocator::Drift(fixed t_0, 
                      const GEOPOINT& location_0, 
                      const GEOPOINT& traildrift,
                      fixed decay)
{
  for (int i = 0; i < TLOCATOR_NMAX; i++) {
    if (points[i].valid)
      points[i].Drift(t_0, location_0, traildrift, decay);
  }
}

void
ThermalLocator::EstimateThermalBase(const GEOPOINT Thermal_Location,
                                    const fixed altitude, const fixed wthermal,
                                    const SpeedVector wind,
                                    GEOPOINT *ground_location, fixed *ground_alt)
{
  if ((Thermal_Location.Longitude == 0.0)
      || (Thermal_Location.Latitude == 0.0)
      || (wthermal < 1.0)) {
    ground_location->Longitude = 0.0;
    ground_location->Latitude = 0.0;
    *ground_alt = -1.0;
    return;
  }

  fixed Tmax;
  Tmax = (altitude / wthermal);
  fixed dt = Tmax / 10;

  terrain.Lock();

  GEOPOINT loc;
  FindLatitudeLongitude(Thermal_Location, wind.bearing, wind.norm * dt, &loc);
  fixed Xrounding = fabs(loc.Longitude - Thermal_Location.Longitude) / 2;
  fixed Yrounding = fabs(loc.Latitude - Thermal_Location.Latitude) / 2;

  for (fixed t = fixed_zero; t <= Tmax; t += dt) {
    FindLatitudeLongitude(Thermal_Location, wind.bearing, wind.norm * t,
                          &loc);

    fixed hthermal = altitude - wthermal * t;
    fixed hground = fixed_zero;

    if (terrain.GetMap()) {
      RasterRounding rounding(*terrain.GetMap(), Xrounding, Yrounding);
      hground = terrain.GetTerrainHeight(loc, rounding);
    }

    fixed dh = hthermal - hground;
    if (dh < 0) {
      t = t + dh / wthermal;
      FindLatitudeLongitude(Thermal_Location, wind.bearing, wind.norm * t,
                            &loc);
      break;
    }
  }

  fixed hground = fixed_zero;
  if (terrain.GetMap()) {
    RasterRounding rounding(*terrain.GetMap(), Xrounding, Yrounding);
    hground = terrain.GetTerrainHeight(loc, rounding);
  }

  terrain.Unlock();

  *ground_location = loc;
  *ground_alt = hground;
}

void
ThermalLocator::Process(const bool circling,
                        const fixed time, 
                        const GEOPOINT &location, 
                        const fixed w,
                        const SpeedVector wind,
                        THERMAL_LOCATOR_INFO& therm)
{
  if (circling) {
    AddPoint(time, location, w);
    Update(time, location, wind, therm);
  } else {
    Reset();
    therm.ThermalEstimate_W = 0;
    therm.ThermalEstimate_R = -1;
  }
}
