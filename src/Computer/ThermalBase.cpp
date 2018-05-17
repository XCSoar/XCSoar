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

#include "ThermalBase.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/RasterMap.hpp"
#include "Geo/Math.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/SpeedVector.hpp"

static double
GetElevation(RasterTerrain::Lease &map, const GeoPoint loc)
{
  return (double)map->GetHeight(loc).GetValueOr0();
}

void
EstimateThermalBase(const RasterTerrain *terrain,
                    const GeoPoint location, const double altitude,
                    const double average, const SpeedVector wind,
                    GeoPoint &ground_location, double &ground_alt)
{
  if (average <= 0 || altitude <= 0) {
    ground_location = location;
    ground_alt = 0;
    return;
  }

  // Max time the thermal could have risen for if ground
  // elevation is zero
  const auto Tmax = altitude / average;

  // Shortcut if no terrain available
  if (terrain == NULL) {
    ground_location = FindLatitudeLongitude(location, 
                                            wind.bearing,
                                            wind.norm * Tmax);
    ground_alt = 0;
    return;
  }

  RasterTerrain::Lease map(*terrain);

  // Height step of the 10 calculation intervals
  const auto dh = altitude / 10;

  // Iterate over 10 altitude-based calculation intervals
  // We do this because the terrain elevation may shift
  // as we trace the thermal back to its source

  GeoPoint loc = location;

  for (auto h = altitude; h >= 0; h -= dh) {
    // Time to descend to this height
    auto t = (altitude - h) / average;

    // Calculate position
    loc = FindLatitudeLongitude(location, wind.bearing, 
                                wind.norm * t);

    // Calculate altitude above ground
    auto dh = h - GetElevation(map, loc);

    // At or below ground level, use linear interpolation
    // to estimate intersection
    if (dh <= 0) {
      // Calculate time when we passed the ground level
      t += dh / average;

      if (t <= 0)
        /* can happen when the terrain at this location is higher than
           the aircraft's current altitude; bail out */
        break;

      // Calculate position
      loc = FindLatitudeLongitude(location, wind.bearing, 
                                  wind.norm * t);
      break;
    }
  }

  ground_location = loc;
  ground_alt = GetElevation(map, ground_location);
}

