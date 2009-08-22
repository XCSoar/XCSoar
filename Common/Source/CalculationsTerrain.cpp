/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "externs.h"
#include "Calculations.h"
#include "GlideSolvers.hpp"
#include "MapWindow.h"
#include "RasterTerrain.h"
#include "Math/Earth.hpp"

void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  short Alt = 0;

  RasterTerrain::Lock();
  // want most accurate rounding here
  RasterTerrain::SetTerrainRounding(0,0);
  Alt = RasterTerrain::GetTerrainHeight(Basic->Latitude,
                                        Basic->Longitude);
  RasterTerrain::Unlock();

  if(Alt<0) {
    Alt = 0;
    if (Alt <= TERRAIN_INVALID) {
      Calculated->TerrainValid = false;
    } else {
      Calculated->TerrainValid = true;
    }
    Calculated->TerrainAlt = 0;
  } else {
    Calculated->TerrainValid = true;
    Calculated->TerrainAlt = Alt;
  }
  Calculated->AltitudeAGL = Calculated->NavAltitude - Calculated->TerrainAlt;
  if (!FinalGlideTerrain) {
    Calculated->TerrainBase = Calculated->TerrainAlt;
  }
}

void CheckFinalGlideThroughTerrain(NMEA_INFO *Basic,
				   DERIVED_INFO *Calculated,
				   double LegToGo,
				   double LegBearing) {

  // Final glide through terrain updates
  if (Calculated->FinalGlide) {

    double lat, lon;
    bool out_of_range;
    double distance_soarable =
      FinalGlideThroughTerrain(LegBearing,
                               Basic, Calculated,
                               &lat,
                               &lon,
                               LegToGo, &out_of_range, NULL);

    if ((!out_of_range)&&(distance_soarable< LegToGo)) {
      Calculated->TerrainWarningLatitude = lat;
      Calculated->TerrainWarningLongitude = lon;
    } else {
      Calculated->TerrainWarningLatitude = 0.0;
      Calculated->TerrainWarningLongitude = 0.0;
    }
  } else {
    Calculated->TerrainWarningLatitude = 0.0;
    Calculated->TerrainWarningLongitude = 0.0;
  }
}


void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {

  if (FinalGlideTerrain) {

    double bearing, distance;
    double lat, lon;
    bool out_of_range;

    // estimate max range (only interested in at most one screen distance away)
    // except we need to scan for terrain base, so 20km search minimum is required
    double mymaxrange = max(20000.0, MapWindow::GetApproxScreenRange());

    Calculated->TerrainBase = Calculated->TerrainAlt;

    for (int i=0; i<=NUMTERRAINSWEEPS; i++) {
      bearing = (i*360.0)/NUMTERRAINSWEEPS;
      distance = FinalGlideThroughTerrain(bearing,
					  Basic,
					  Calculated, &lat, &lon,
					  mymaxrange, &out_of_range,
					  &Calculated->TerrainBase);
      if (out_of_range) {
	FindLatitudeLongitude(Basic->Latitude, Basic->Longitude,
			      bearing,
			      mymaxrange*20,
			      &lat, &lon);
      }
      Calculated->GlideFootPrint[i].x = lon;
      Calculated->GlideFootPrint[i].y = lat;
    }
    Calculated->Experimental = Calculated->TerrainBase;
  }

  static double LastOptimiseTime = 0;
  // moved from Calculations.cpp

  if(Basic->Time> LastOptimiseTime+0.0)
    {
      LastOptimiseTime = Basic->Time;
      RasterTerrain::ServiceCache();
    }
}
