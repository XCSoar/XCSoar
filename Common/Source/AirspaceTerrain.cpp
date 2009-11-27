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

#include "AirspaceDatabase.hpp"
#include "RasterTerrain.h"
#include "RasterMap.h"

static void
UpdateAGL(AIRSPACE_ALT &altitude, const GEOPOINT &location,
          const RasterTerrain &terrain, const RasterRounding &rounding)
{
  if (altitude.Base != abAGL)
    return;

  double terrain_height = terrain.GetTerrainHeight(location, rounding);

  if (altitude.AGL>=0) {
    altitude.Altitude = altitude.AGL + terrain_height;
  } else {
    // surface, set to zero
    altitude.AGL = 0;
    altitude.Altitude = 0;
  }
}

static void
UpdateAGL(AirspaceMetadata &airspace, const GEOPOINT &location,
          const RasterTerrain &terrain, const RasterRounding &rounding)
{
  UpdateAGL(airspace.Base, location, terrain, rounding);
  UpdateAGL(airspace.Top, location, terrain, rounding);
}

void
AirspaceDatabase::UpdateAGL(const RasterTerrain &terrain)
{
  const RasterMap *map = terrain.GetMap();
  if (map == NULL)
    /* XXX: complain */
    return;

  // want most accurate rounding here
  const RasterRounding rounding(*map, 0, 0);

  for (unsigned i = 0; i < NumberOfAirspaceAreas; i++) {
    AIRSPACE_AREA &area = AirspaceArea[i];
    GEOPOINT center;

    center.Latitude = (area.maxBound.Latitude + area.minBound.Latitude) / 2;
    center.Longitude = (area.maxBound.Longitude + area.minBound.Longitude) / 2;

    ::UpdateAGL(AirspaceArea[i], center, terrain, rounding);
  }

  for (unsigned i = 0; i < NumberOfAirspaceCircles; i++) {
    AIRSPACE_CIRCLE &circle = AirspaceCircle[i];
    ::UpdateAGL(circle, circle.Location, terrain, rounding);
  }
}
