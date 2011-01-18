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

#ifndef XCSOAR_GLIDE_TERRAIN_HPP
#define XCSOAR_GLIDE_TERRAIN_HPP

#include <stddef.h>

struct AIRCRAFT_STATE;
struct SETTINGS_COMPUTER;
class RasterTerrain;
class RasterMap;
class GlidePolar;
struct GlideResult;
class RasterRounding;

#include "Math/fixed.hpp"
#include "Navigation/GeoPoint.hpp"

struct TerrainIntersection
{
  TerrainIntersection(const GeoPoint& start);

  GeoPoint location;
  fixed range;
  fixed altitude;
  bool out_of_range;
};

class GlideTerrain
{
public:
  GlideTerrain(const SETTINGS_COMPUTER &settings,
               RasterTerrain &terrain);

  void set_max_range(const fixed set);

  fixed get_terrain_base() const;

  /** 
   * Find intersection for pure glide
   * 
   * @param basic State of aircraft at origin
   * @param polar Glide polar for descent
   * 
   * @return Intersection
   */
  TerrainIntersection find_intersection(const AIRCRAFT_STATE &basic,
                                        const GlidePolar& polar);

  /** 
   * Find intersection for cruise (no height loss)
   * 
   * @param basic State of aircraft at origin
   * 
   * @return Intersection
   */
  TerrainIntersection find_intersection(const AIRCRAFT_STATE &basic);

  /** 
   * Find intersection for compound glide solution
   * 
   * @param basic State of aircraft at origin
   * @param solution Glide solution describing cruise-climb and descent
   * @param polar Glide polar for descent portion
   * 
   * @return Intersection
   */
  TerrainIntersection find_intersection(const AIRCRAFT_STATE &basic,
                                        const GlideResult& solution,
                                        const GlidePolar& polar);

private:
  fixed h_terrain(const RasterMap &map, const GeoPoint& loc);

  RasterTerrain &m_terrain;
  const fixed safety_height_terrain;
  fixed TerrainBase;
  fixed max_range;
};

#endif
