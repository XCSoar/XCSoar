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
#include "Math/Earth.hpp"
#include "Navigation/TaskProjection.hpp"
#include <math.h>

void
ThermalLocator::ThermalLocator_Point::Drift(fixed t, 
                                            const TaskProjection& projection,
                                            const GeoPoint& wind_drift,
                                            fixed decay)
{
  static const fixed decay_factor(-1.5/TLOCATOR_NMAX);
  // convert to flat earth coordinates, then drift by wind and delta t
  const fixed dt = t - t_0;

  weight = exp(decay_factor * decay * dt);

  GeoPoint p = location+wind_drift*dt;

  loc_drift = projection.fproject(p);
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
ThermalLocator::AddPoint(const fixed t, const GeoPoint &location, const fixed w)
{
  points[n_index].location = location;
  points[n_index].t_0 = t;
  points[n_index].w = max(w, fixed(-0.1));
  points[n_index].valid = true;

  n_index = (n_index+1) % TLOCATOR_NMAX;

  if (n_points+1 < TLOCATOR_NMAX)
    n_points++;

  if (!initialised) {
    initialised = true;

    // set initial estimate
    est_location = location;
    est_t = t;
  }
}

void
ThermalLocator::invalid_estimate(THERMAL_LOCATOR_INFO &therm) const
{
  therm.ThermalEstimate_R = fixed_minus_one;
  therm.ThermalEstimate_W = fixed_zero;
}

void
ThermalLocator::Update(const fixed t_0, 
                       const GeoPoint &location_0,
                       const SpeedVector wind, 
                       THERMAL_LOCATOR_INFO &therm)
{
  if (n_points < TLOCATOR_NMIN) {
    invalid_estimate(therm);
    return; // nothing to do.
  }

  GeoPoint dloc = FindLatitudeLongitude(location_0, wind.bearing, wind.norm);

  const GeoPoint traildrift = location_0 - dloc;

  TaskProjection projection;
  projection.reset(location_0);
  projection.update_fast();

  Update_Internal(t_0, 
                  projection, 
                  location_0,
                  traildrift, 
                  fixed_one, therm);
}

void
ThermalLocator::glider_average(fixed &xav, fixed& yav) 
{
  // find glider's average position
  int acc = 0;
  for (int i = 0; i < TLOCATOR_NMAX; ++i) {
    if (points[i].valid) {
      xav += points[i].loc_drift.x;
      yav += points[i].loc_drift.y;
      acc++;
    }
  }
  if (acc) {
    xav/= acc;
    yav/= acc;
  }
}


void
ThermalLocator::Update_Internal(const fixed t_0, 
                                const TaskProjection& projection, 
                                const GeoPoint& location_0,
                                const GeoPoint& traildrift,
                                const fixed decay, 
                                THERMAL_LOCATOR_INFO &therm)
{
  // drift points 
  Drift(t_0, projection, traildrift, decay);

  fixed xav = fixed_zero;
  fixed yav = fixed_zero;

  glider_average(xav, yav);

  // find thermal center relative to glider's average position

  fixed acc = fixed_zero;
  fixed sx = fixed_zero;
  fixed sy = fixed_zero;

  for (int i = 0; i < TLOCATOR_NMAX; ++i) {
    if (points[i].valid) {
      const fixed weight = points[i].w * points[i].weight;
      sx += (points[i].loc_drift.x - xav) * weight;
      sy += (points[i].loc_drift.y - yav) * weight;
      acc += weight;
    }
  }

  est_t = t_0;

  // if sufficient data, estimate location

  if (!positive(acc)) {
    invalid_estimate(therm);
    return;
  }

  const FlatPoint f0(sx/acc+xav, sy/acc+yav);
  est_location = projection.funproject(f0);
  
  therm.ThermalEstimate_Location = est_location;
  therm.ThermalEstimate_R = fixed_one;
  therm.ThermalEstimate_W = fixed_one;
}

void
ThermalLocator::Drift(const fixed t_0, 
                      const TaskProjection& projection, 
                      const GeoPoint& traildrift,
                      const fixed decay)
{
  for (int i = 0; i < TLOCATOR_NMAX; ++i) {
    if (points[i].valid)
      points[i].Drift(t_0, projection, traildrift, decay);
  }
}

void
ThermalLocator::Process(const bool circling,
                        const fixed time, 
                        const GeoPoint &location, 
                        const fixed w,
                        const SpeedVector wind,
                        THERMAL_LOCATOR_INFO& therm)
{
  if (circling) {
    AddPoint(time, location, w);
    Update(time, location, wind, therm);
  } else {
    Reset();
    invalid_estimate(therm);
  }
}
