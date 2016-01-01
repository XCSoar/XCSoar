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

#include "NMEA/ThermalLocator.hpp"
#include "Geo/SpeedVector.hpp"
#include "Geo/Math.hpp"

#include <algorithm>

void
ThermalLocatorInfo::Clear()
{
  estimate_valid = false;

  // clear thermal sources for first time.
  sources.clear();
}

static inline bool
CompareTime(const ThermalSource &a, const ThermalSource &b)
{
  return a.time < b.time;
}

ThermalSource &
ThermalLocatorInfo::AllocateSource()
{
  if (!sources.full())
    return sources.append();

  auto oldest = std::min_element(sources.begin(), sources.end(),
                                 CompareTime);
  assert(oldest != sources.end());
  return *oldest;
}

GeoPoint
ThermalSource::CalculateAdjustedLocation(double altitude,
                                         const SpeedVector &wind) const
{
  auto dh = altitude - ground_height;
  auto t = dh / lift_rate;
  return FindLatitudeLongitude(location, wind.bearing.Reciprocal(), wind.norm * t);
}
