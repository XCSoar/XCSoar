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

#include "GlideSolvers.hpp"
#include "MacCready.h"
#include "SettingsComputer.hpp"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "GeoPoint.hpp"

double
FinalGlideThroughTerrain(const double this_bearing,
                         const NMEA_INFO &basic,
                         const DERIVED_INFO &calculated,
                         const SETTINGS_COMPUTER &settings,
                         const RasterTerrain &terrain,
                         GEOPOINT *retloc,
                         const double max_range,
                         bool *out_of_range,
                         double *TerrainBase)
{
  double mc = GlidePolar::GetMacCready();
  double irange = GlidePolar::MacCreadyAltitude(mc,
						1.0, this_bearing,
                                                calculated.WindSpeed,
                                                calculated.WindBearing,
						0, 0, true, 0);
  const GEOPOINT start_loc = basic.Location;
  if (retloc) {
    *retloc = start_loc;
  }
  *out_of_range = false;

  if (irange <= 0.0 || calculated.NavAltitude <= 0)
    // can't make progress in this direction at the current windspeed/mc
    return 0;

  const RasterMap *map = terrain.GetMap();
  if (map == NULL)
    return 0;

  const double glide_max_range = calculated.NavAltitude/irange;

  // returns distance one would arrive at altitude in straight glide
  // first estimate max range at this altitude
  GEOPOINT loc, last_loc;
  double h=0.0, dh=0.0;
  // int imax=0;
  double last_dh=0;
  double altitude;

  double retval = 0;
  int i=0;
  bool start_under = false;

  // calculate terrain rounding factor

  FindLatitudeLongitude(start_loc, 0,
                        glide_max_range/NUMFINALGLIDETERRAIN, &loc);

  double Xrounding = fabs(loc.Longitude-start_loc.Longitude)/2;
  double Yrounding = fabs(loc.Latitude-start_loc.Latitude)/2;
  const RasterRounding rounding(*map, Xrounding, Yrounding);

  loc = last_loc = start_loc;

  altitude = calculated.NavAltitude;
  h =  max(0, terrain.GetTerrainHeight(loc,rounding));
  dh = altitude - h - settings.SafetyAltitudeTerrain;
  last_dh = dh;
  if (dh<0) {
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

  double f_scale = 1.0/NUMFINALGLIDETERRAIN;
  if ((max_range>0) && (max_range<glide_max_range)) {
    f_scale *= max_range/glide_max_range;
  }

  double delta_alt = -f_scale * calculated.NavAltitude;

  dloc.Latitude *= f_scale;
  dloc.Longitude *= f_scale;

  for (i=1; i<=NUMFINALGLIDETERRAIN; i++) {
    double f;
    bool solution_found = false;
    double fi = i*f_scale;
    // fraction of glide_max_range

    if ((max_range>0)&&(fi>=1.0)) {
      // early exit
      *out_of_range = true;
      return max_range;
    }

    if (start_under) {
      altitude += 2.0*delta_alt;
    } else {
      altitude += delta_alt;
    }

    // find lat, lon of point of interest

    loc.Latitude += dloc.Latitude;
    loc.Longitude += dloc.Longitude;

    // find height over terrain
    h =  max(0,terrain.GetTerrainHeight(loc, rounding));

    dh = altitude - h - settings.SafetyAltitudeTerrain;

    if (TerrainBase && (dh>0) && (h>0)) {
      *TerrainBase = min(*TerrainBase, h);
    }

    if (start_under) {
      if (dh>last_dh) {
        // better solution found, ok to continue...
        if (dh>0) {
          // we've now found a terrain point above safety altitude,
          // so consider rest of track to search for safety altitude
          start_under = false;
        }
      } else {
        f= 0.0;
        solution_found = true;
      }
    } else if (dh<=0) {
      if ((dh<last_dh) && (last_dh>0)) {
        f = max(0,min(1,(-last_dh)/(dh-last_dh)));
      } else {
        f = 0.0;
      }
      solution_found = true;
    }
    if (solution_found) {
      loc.Latitude = last_loc.Latitude*(1.0-f)+loc.Latitude*f;
      loc.Longitude = last_loc.Longitude*(1.0-f)+loc.Longitude*f;
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
