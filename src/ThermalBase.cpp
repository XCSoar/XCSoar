/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

void
EstimateThermalBase(const GeoPoint location, const fixed altitude,
                    const fixed average, const SpeedVector wind,
                    GeoPoint &ground_location, fixed &ground_alt)
{
  if (location.Longitude == Angle::native(fixed_zero) ||
      location.Latitude == Angle::native(fixed_zero) ||
      average < fixed_one) {
    ground_location.Longitude = Angle::native(fixed_zero);
    ground_location.Latitude = Angle::native(fixed_zero);
    ground_alt = fixed_minus_one;
    return;
  }

  fixed Tmax = altitude / average;
  fixed dt = Tmax / 10;

  RasterTerrain::Lease *map = (terrain != NULL) ?
                              new RasterTerrain::Lease(*terrain) : NULL;

  GeoPoint loc;
  for (fixed t = fixed_zero; t <= Tmax; t += dt) {
    loc = FindLatitudeLongitude(location, wind.bearing, wind.norm * t);

    fixed hthermal = altitude - average * t;
    short hground = (map != NULL) ? (*map)->GetField(loc) :
                                    RasterTerrain::TERRAIN_INVALID;
    if (RasterBuffer::is_special(hground))
      hground = 0;

    fixed dh = hthermal - fixed(hground);
    if (negative(dh)) {
      t = t + dh / average;
      loc = FindLatitudeLongitude(location, wind.bearing, wind.norm * t);
      break;
    }
  }

  short hground = (map != NULL) ? (*map)->GetField(loc) :
                                  RasterTerrain::TERRAIN_INVALID;
  if (RasterBuffer::is_special(hground))
    hground = 0;

  delete map;

  ground_location = loc;
  ground_alt = fixed(hground);
}

