/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Store.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

#include <math.h>

void
WindStore::reset()
{
  windlist.Reset();
  update_clock = 0;
  updated = true;
}

void
WindStore::SlotMeasurement(const MoreData &info,
                           const SpeedVector &windvector, unsigned quality)
{
  updated = true;
  windlist.addMeasurement((unsigned)info.time, windvector,
                          info.nav_altitude, quality);
  update_clock = info.clock;
}

void
WindStore::SlotAltitude(const MoreData &info, DerivedInfo &derived)
{
  if (updated || (fabs(info.nav_altitude - _lastAltitude) > 100)) {
    //only recalculate if there is a significant change
    recalculateWind(info, derived);

    updated = false;
    _lastAltitude = info.nav_altitude;
  }
}

const Vector
WindStore::GetWind(double Time, double h, bool &found) const
{
  return windlist.getWind((unsigned)Time, h, found);
}

void
WindStore::recalculateWind(const MoreData &info, DerivedInfo &derived) const
{
  bool found;
  Vector CurWind = windlist.getWind((unsigned)info.time, info.nav_altitude,
                                    found);

  if (found) {
    NewWind(info, derived, CurWind);
  }
}

void
WindStore::NewWind(const NMEAInfo &info, DerivedInfo &derived,
                   Vector &wind) const
{
  auto mag = wind.Magnitude();
  Angle bearing;

  if (wind.y == 0 && wind.x == 0)
    bearing = Angle::Zero();
  else
    bearing = Angle::FromXY(wind.x, wind.y);

  if (mag < 30) { // limit to reasonable values
    derived.estimated_wind = SpeedVector(bearing.AsBearing(), mag);
    derived.estimated_wind_available.Update(update_clock);
  } else {
    // TODO code: give warning, wind estimate bogus or very strong!
  }

  #ifdef DEBUG_WIND
  LogDebug(_T("%f %f 0 # wind estimate\n"), wind.x, wind.y);
  #endif

}
