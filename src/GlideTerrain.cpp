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
#include "Navigation/Aircraft.hpp"

#include "Navigation/Geometry/GeoVector.hpp"
#include "GlideSolvers/GlidePolar.hpp"

fixed
FinalGlideThroughTerrain(const AIRCRAFT_STATE &basic,
                         const GlidePolar& polar,
                         const SETTINGS_COMPUTER &settings,
                         const RasterTerrain &terrain,
                         GEOPOINT *retloc,
                         const fixed max_range,
                         bool *out_of_range,
                         fixed *TerrainBase)
{
  const fixed ld = polar.get_ld_over_ground(basic);

  const GEOPOINT start_loc = basic.Location;
  if (retloc) {
    *retloc = start_loc;
  }
  *out_of_range = false;

  if (!positive(ld) || !positive(basic.NavAltitude))
    // can't make progress in this direction at the current windspeed/mc
    return fixed_zero;

  const RasterMap *map = terrain.GetMap();
  if (map == NULL)
    return fixed_zero;

  const fixed glide_max_range = basic.NavAltitude*ld;

  // returns distance one would arrive at altitude in straight glide
  // first estimate max range at this altitude

  // calculate terrain rounding factor

  GeoVector vec(glide_max_range/NUMFINALGLIDETERRAIN, basic.TrackBearing);
  const RasterRounding rounding(*map, (vec.end_point(start_loc)-start_loc)*fixed_half);

  fixed h= fixed_zero, dh= fixed_zero;
  fixed last_dh=fixed_zero;
  fixed altitude = basic.NavAltitude;

  GEOPOINT loc= start_loc, last_loc = start_loc;

  h = max(fixed_zero, fixed(terrain.GetTerrainHeight(loc, rounding)));
  dh = altitude - h - settings.SafetyAltitudeTerrain;
  last_dh = dh;

  bool start_under = negative(dh);

  fixed f_scale = fixed_one/NUMFINALGLIDETERRAIN;
  if (positive(max_range) && (max_range<glide_max_range)) {
    f_scale *= max_range/glide_max_range;
  }

  // find grid
  vec.Distance = glide_max_range*f_scale;
  GEOPOINT dloc = vec.end_point(start_loc)-start_loc;

  const fixed delta_alt = -f_scale * basic.NavAltitude;

  for (int i=1; i<=NUMFINALGLIDETERRAIN; i++) {
    fixed f;
    bool solution_found = false;
    const fixed fi = i*f_scale;
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
    h = max(fixed_zero, fixed(terrain.GetTerrainHeight(loc, rounding)));

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
        f = fixed_zero;
      }
      solution_found = true;
    }
    if (solution_found) {
      loc = loc.interpolate(last_loc, f);
      if (retloc) {
        *retloc = loc;
      }
      return loc.distance(start_loc);
    }
    last_dh = dh;
    last_loc = loc;
  }

  *out_of_range = true;

  return glide_max_range;
}
