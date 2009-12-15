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

/**
 * Final glide through terrain and footprint calculations
 * @file GlideSolvers.cpp
 */

#include "MacCready.h"
#include "SettingsComputer.hpp"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"

fixed
FinalGlideThroughTerrain(const fixed this_bearing,
                         const NMEA_INFO &basic,
                         const DERIVED_INFO &calculated,
                         const SETTINGS_COMPUTER &settings,
                         const RasterTerrain &terrain,
                         GEOPOINT *retloc,
                         const fixed max_range,
                         bool *out_of_range,
                         fixed *TerrainBase)
{
  fixed mc = oldGlidePolar::GetMacCready();
  fixed irange = oldGlidePolar::MacCreadyAltitude(mc,
						1.0, this_bearing,
                                                calculated.WindSpeed,
                                                calculated.WindBearing,
						0, 0, true, 0);
  const GEOPOINT start_loc = basic.Location;
  if (retloc) {
    *retloc = start_loc;
  }
  *out_of_range = false;

  if (!positive(irange) || !positive(calculated.NavAltitude))
    // can't make progress in this direction at the current windspeed/mc
    return 0;

  const RasterMap *map = terrain.GetMap();
  if (map == NULL)
    return 0;

  const fixed glide_max_range = calculated.NavAltitude/irange;

  // returns distance one would arrive at altitude in straight glide
  // first estimate max range at this altitude
  GEOPOINT loc, last_loc;
  fixed h= fixed_zero, dh= fixed_zero;
  // int imax=0;
  fixed last_dh=fixed_zero;
  fixed altitude;

  fixed retval = fixed_zero;
  bool start_under = false;

  // calculate terrain rounding factor

  FindLatitudeLongitude(start_loc, fixed_zero,
                        glide_max_range/NUMFINALGLIDETERRAIN, &loc);

  fixed Xrounding = fabs(loc.Longitude-start_loc.Longitude)/2;
  fixed Yrounding = fabs(loc.Latitude-start_loc.Latitude)/2;
  const RasterRounding rounding(*map, Xrounding, Yrounding);

  loc = last_loc = start_loc;

  altitude = calculated.NavAltitude;
  h =  max(fixed_zero, terrain.GetTerrainHeight(loc,rounding));
  dh = altitude - h - settings.SafetyAltitudeTerrain;
  last_dh = dh;
  if (negative(dh)) {
    start_under = true;
    // already below safety terrain height
    //    retval = 0;
    //    goto OnExit;
  }

  // find grid
  GEOPOINT dloc;

  FindLatitudeLongitude(loc, this_bearing, glide_max_range, &dloc);
  dloc.Latitude -= start_loc.Latitude;
  dloc.Longitude -= start_loc.Longitude;

  fixed f_scale = fixed_one/NUMFINALGLIDETERRAIN;
  if (positive(max_range) && (max_range<glide_max_range)) {
    f_scale *= max_range/glide_max_range;
  }

  fixed delta_alt = -f_scale * calculated.NavAltitude;

  dloc.Latitude *= f_scale;
  dloc.Longitude *= f_scale;

  for (int i=1; i<=NUMFINALGLIDETERRAIN; i++) {
    fixed f;
    bool solution_found = false;
    fixed fi = i*f_scale;
    // fraction of glide_max_range

    if (positive(max_range)&&(fi>=fixed_one)) {
      // early exit
      *out_of_range = true;
      return max_range;
    }

    if (start_under) {
      altitude += fixed_two*delta_alt;
    } else {
      altitude += delta_alt;
    }

    // find lat, lon of point of interest

    loc.Latitude += dloc.Latitude;
    loc.Longitude += dloc.Longitude;

    // find height over terrain
    h =  max(fixed_zero,terrain.GetTerrainHeight(loc, rounding));

    dh = altitude - h - settings.SafetyAltitudeTerrain;

    if (TerrainBase && positive(dh) && positive(h)) {
      *TerrainBase = min(*TerrainBase, h);
    }

    if (start_under) {
      if (dh>last_dh) {
        // better solution found, ok to continue...
        if (positive(dh)) {
          // we've now found a terrain point above safety altitude,
          // so consider rest of track to search for safety altitude
          start_under = false;
        }
      } else {
        f= fixed_zero;
        solution_found = true;
      }
    } else if (!positive(dh)) {
      if ((dh<last_dh) && positive(last_dh)) {
        f = max(fixed_zero,min(fixed_one,(-last_dh)/(dh-last_dh)));
      } else {
        f = 0;
      }
      solution_found = true;
    }
    if (solution_found) {
      loc.Latitude = last_loc.Latitude*(fixed_one-f)+loc.Latitude*f;
      loc.Longitude = last_loc.Longitude*(fixed_one-f)+loc.Longitude*f;
      if (retloc) {
        *retloc = loc;
      }
      return Distance(start_loc, loc);
    }
    last_dh = dh;
    last_loc = loc;
  }

  *out_of_range = true;
  retval = glide_max_range;

  return retval;
}
