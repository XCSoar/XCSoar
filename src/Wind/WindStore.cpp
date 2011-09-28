/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

/* This library was originally imported from Cumulus
   http://kflog.org/cumulus/ */

#include "Wind/WindStore.hpp"
#include "Math/Constants.h"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

#include <math.h>

WindStore::WindStore()
{
  //create the lists
  updated = true;
}

void
WindStore::reset()
{
  windlist.Reset();
  updated = true;
  _lastAltitude = fixed_zero;
}

void
WindStore::SlotMeasurement(const MoreData &info,
                           Vector windvector, int quality)
{
  updated = true;
  windlist.addMeasurement(info.time, windvector, info.nav_altitude, quality);
}

void
WindStore::SlotAltitude(const MoreData &info, DerivedInfo &derived)
{
  if ((fabs(info.nav_altitude - _lastAltitude) > fixed(100)) || updated) {
    //only recalculate if there is a significant change
    recalculateWind(info, derived);

    updated = false;
    _lastAltitude = info.nav_altitude;
  }
}

const Vector
WindStore::GetWind(fixed Time, fixed h, bool &found) const
{
  return windlist.getWind(Time, h, found);
}

void
WindStore::recalculateWind(const MoreData &info, DerivedInfo &derived) const
{
  bool found;
  Vector CurWind = windlist.getWind(info.time, info.nav_altitude, found);

  if (found) {
    NewWind(info, derived, CurWind);
  }
}

void
WindStore::NewWind(const NMEAInfo &info, DerivedInfo &derived,
                   Vector &wind) const
{
  fixed mag = hypot(wind.x, wind.y);
  Angle bearing;

  if (wind.y == fixed_zero && wind.x == fixed_zero)
    bearing = Angle::zero();
  else
    bearing = Angle::radians(atan2(wind.y, wind.x));

  if (mag < fixed(30)) { // limit to reasonable values
    derived.estimated_wind = SpeedVector(bearing.as_bearing(), mag);
    derived.estimated_wind_available.Update(info.clock);
  } else {
    // TODO code: give warning, wind estimate bogus or very strong!
  }

  #ifdef DEBUG_WIND
  LogDebug(_T("%f %f 0 # wind estimate\n"), wind.x, wind.y);
  #endif

}
