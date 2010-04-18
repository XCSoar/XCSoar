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
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include <math.h>

#define SFACT 111195

void
ThermalLocator::ThermalLocator_Point::Drift(fixed t, 
                                            const GEOPOINT& location_0,
                                            const GEOPOINT& wind_drift,
                                            fixed decay)
{
  static const fixed decay_factor(-1.5/TLOCATOR_NMAX);
  // convert to flat earth coordinates, then drift by wind and delta t
  const fixed dt = t - t_0;

  fixed x = (location.Longitude + wind_drift.Longitude * dt - location_0.Longitude) 
    * fastcosine(location_0.Latitude);
  fixed y = (location.Latitude + wind_drift.Latitude * dt - location_0.Latitude);

  weight = iround(100*exp(decay_factor * decay * dt));
  x_weighted = iround(x * SFACT * weight);
  y_weighted = iround(y * SFACT * weight);
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
    for (int i = 0; i < TLOCATOR_NMAX; ++i) {
      points[i].valid = false;
    }
    n_index = 0;
    n_points = 0;
  }
}

void
ThermalLocator::AddPoint(const fixed t, const GEOPOINT &location, const fixed w)
{
  points[n_index].location = location;
  points[n_index].t_0 = t;
  points[n_index].w_scaled = iround(max(w, fixed(-0.1)) * 10);
  points[n_index].valid = true;

  n_index++;
  n_index = (n_index % TLOCATOR_NMAX);

  if (n_points < TLOCATOR_NMAX - 1)
    n_points++;

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
ThermalLocator::Update(const fixed t_0, 
                       const GEOPOINT &location_0,
                       const SpeedVector wind, 
                       THERMAL_LOCATOR_INFO &therm)
{
  if (n_points < TLOCATOR_NMIN) {
    invalid_estimate(therm);
    return; // nothing to do.
  }

  GEOPOINT dloc;

  FindLatitudeLongitude(location_0, wind.bearing, wind.norm, &dloc);

  const GEOPOINT traildrift = location_0-dloc;

  // drift estimate from previous time step
  const fixed dt = t_0 - est_t;
  est_location += traildrift * dt;
  est_x = (est_location.Longitude - location_0.Longitude) * fastcosine(location_0.Latitude);
  est_y = (est_location.Latitude - location_0.Latitude);

  Update_Internal(t_0, 
                  location_0, 
                  traildrift, 
                  fixed_one, therm);
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

  int acc = 0;
  int sx = 0;
  int sy = 0;
  int i;

  // xav, yav is average glider's position
  int xav = 0;
  int yav = 0;

  // find glider's average position

  for (i = 0; i < TLOCATOR_NMAX; ++i) {
    if (points[i].valid) {
      xav += points[i].x_weighted;
      yav += points[i].y_weighted;
      acc += points[i].weight;
    }
  }
  xav /= acc;
  yav /= acc;

  // find thermal center relative to glider's average position

  acc = 0;
  for (i = 0; i < TLOCATOR_NMAX; ++i) {
    if (points[i].valid) {
      sx += (points[i].x_weighted - xav * points[i].weight) * points[i].w_scaled;
      sy += (points[i].y_weighted - yav * points[i].weight) * points[i].w_scaled;
      acc += points[i].w_scaled * points[i].weight;
    }
  }

  // if sufficient data, estimate location

  if (acc > 0.25) {
    sx /= acc;
    sy /= acc;

    est_x = (sx + xav) / SFACT;
    est_y = (sy + yav) / SFACT;

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
  for (int i = 0; i < TLOCATOR_NMAX; ++i) {
    if (points[i].valid)
      points[i].Drift(t_0, location_0, traildrift, decay);
  }
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
