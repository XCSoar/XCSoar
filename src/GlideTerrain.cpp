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

#include "GlideTerrain.hpp"
#include "SettingsComputer.hpp"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "Navigation/Aircraft.hpp"

#include "Navigation/Geometry/GeoVector.hpp"
#include "GlideSolvers/GlidePolar.hpp"

TerrainIntersection::TerrainIntersection(const GEOPOINT& start):
  location(start),
  range(fixed_zero),
  altitude(fixed_zero),
  out_of_range(false)
{};

GlideTerrain::GlideTerrain(const SETTINGS_COMPUTER &settings,
                           RasterTerrain &terrain):
  m_terrain(terrain),
  SafetyAltitudeTerrain(settings.SafetyAltitudeTerrain),
  TerrainBase(fixed_zero),
  max_range(-fixed_one)
{
  if (const RasterMap *map = m_terrain.GetMap()) {
    rounding = new RasterRounding(*map, 0, 0);
  }
}

GlideTerrain::~GlideTerrain() 
{
  if (rounding) {
    delete rounding;
  }
}

void 
GlideTerrain::set_max_range(const fixed set) 
{
  max_range = set;
}

fixed 
GlideTerrain::get_terrain_base() const 
{
  return TerrainBase;
}

bool 
GlideTerrain::valid() const 
{
  return (rounding!=NULL);
}

TerrainIntersection 
GlideTerrain::find_intersection(const AIRCRAFT_STATE &state,
                                const GlidePolar& polar) 
{
  TerrainIntersection retval(state.Location);

  if (!valid()) {
    // no terrain!
    return retval;
  }
  
  const fixed glide_max_range = state.NavAltitude*polar.get_ld_over_ground(state);
  
  if (!positive(glide_max_range)) {
    // can't make progress in this direction at the current windspeed/mc
    return retval;
  }
  
  GEOPOINT loc= state.Location, last_loc = loc;
  fixed altitude = state.NavAltitude;
  fixed h = h_terrain(loc);
  fixed dh = altitude - h;
  fixed last_dh = dh;
  bool start_under = negative(dh);
  
  fixed f_scale = fixed_one/NUMFINALGLIDETERRAIN;
  if (positive(max_range) && (max_range<glide_max_range)) {
    f_scale *= max_range/glide_max_range;
  }
  
  // find grid
  const GeoVector vec(glide_max_range*f_scale, state.TrackBearing);
  const GEOPOINT delta_ll = vec.end_point(state.Location)-state.Location;
  const fixed delta_alt = -f_scale * state.NavAltitude;
  
  for (int i=1; i<=NUMFINALGLIDETERRAIN; ++i) {
    fixed f;
    const fixed fi = i*f_scale;
    // fraction of glide_max_range
    
    if (positive(max_range)&&(fi>=fixed_one)) {
      // early exit
      retval.out_of_range = true;
      return retval;
    }
    
    if (start_under) {
      altitude += fixed_two*delta_alt;
    } else {
      altitude += delta_alt;
    }
    
    // find lat, lon of point of interest
    loc += delta_ll;
    
    // find height over terrain
    h = h_terrain(loc);
    dh = altitude - h;
    
    if (positive(dh) && positive(h)) {
      TerrainBase = min(TerrainBase, h);
    }
    
    bool solution_found = false;
    
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
      retval.location = last_loc.interpolate(loc, f);
      retval.range = retval.location.distance(state.Location);
      retval.altitude = h; // approximation
      return retval;
    }
    last_dh = dh;
    last_loc = loc;
  }
    
  retval.out_of_range = true;
  GeoVector long_vec(max(glide_max_range,max_range)*fixed_two, state.TrackBearing);
  retval.location = long_vec.end_point(state.Location);
  return retval;
}

fixed 
GlideTerrain::h_terrain(const GEOPOINT& loc) 
{
  return max(fixed_zero, fixed(m_terrain.GetTerrainHeight(loc, *rounding)))+SafetyAltitudeTerrain;
}
