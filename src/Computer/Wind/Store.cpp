// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* This library was originally imported from Cumulus
   http://kflog.org/cumulus/ */

#include "Store.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

#include <math.h>

void
WindStore::reset() noexcept
{
  windlist.Reset();
  update_clock = {};
  updated = true;
}

void
WindStore::SlotMeasurement(const MoreData &info,
                           const SpeedVector &windvector, unsigned quality) noexcept
{
  updated = true;
  windlist.addMeasurement(info.time, windvector,
                          info.nav_altitude, quality);
  update_clock = info.clock;
}

void
WindStore::SlotAltitude(const MoreData &info, DerivedInfo &derived) noexcept
{
  if (updated || (fabs(info.nav_altitude - _lastAltitude) > 100)) {
    //only recalculate if there is a significant change
    recalculateWind(info, derived);

    updated = false;
    _lastAltitude = info.nav_altitude;
  }
}

const Vector
WindStore::GetWind(TimeStamp Time, double h,
                   bool &found) const noexcept
{
  return windlist.getWind(Time, h, found);
}

inline void
WindStore::recalculateWind(const MoreData &info, DerivedInfo &derived) const noexcept
{
  bool found;
  Vector CurWind = windlist.getWind(info.time, info.nav_altitude, found);
  if (found) {
    NewWind(derived, CurWind);
  }
}

inline void
WindStore::NewWind(DerivedInfo &derived,
                   const Vector &wind) const noexcept
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
}
