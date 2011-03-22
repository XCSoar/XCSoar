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

#include "ThermalBase.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/RasterMap.hpp"
#include "Components.hpp"
#include "Math/Earth.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/Navigation/SpeedVector.hpp"

static fixed
GetElevation(RasterTerrain::Lease &map, const GeoPoint loc)
{
  short hground = map->GetHeight(loc);
  if (RasterBuffer::is_special(hground))
    hground = 0;

  return fixed(hground);
}

void
EstimateThermalBase(const GeoPoint location, const fixed altitude,
                    const fixed average, const SpeedVector wind,
                    GeoPoint &ground_location, fixed &ground_alt)
{
  if (!positive(average) || !positive(altitude)) {    
    ground_location = location;
    ground_alt = fixed_zero;
    return;
  }

  // Max time the thermal could have risen for if ground
  // elevation is zero
  const fixed Tmax = altitude / average;

  // Shortcut if no terrain available
  if (terrain == NULL) {
    ground_location = FindLatitudeLongitude(location, 
                                            wind.bearing,
                                            wind.norm * Tmax);
    ground_alt = fixed_zero;
    return;
  }

  RasterTerrain::Lease map(*terrain);

  // Height step of the 10 calculation intervals
  const fixed dh = altitude / 10;

  // Iterate over 10 altitude-based calculation intervals
  // We do this because the terrain elevation may shift
  // as we trace the thermal back to its source

  GeoPoint loc = location;

  for (fixed h = altitude; !negative(h); h -= dh) {
    // Time to descend to this height
    fixed t = (altitude-h)/average;

    // Calculate position
    loc = FindLatitudeLongitude(location, wind.bearing, 
                                wind.norm * t);

    // Calculate altitude above ground
    fixed dh = h - GetElevation(map, loc);

    // At or below ground level, use linear interpolation
    // to estimate intersection
    if (!positive(dh)) {
      // Calculate time when we passed the ground level
      t += dh / average;

      if (!positive(t))
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

