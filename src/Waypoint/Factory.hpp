/* Copyright_License {

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

#ifndef WAYPOINT_FACTORY_HPP
#define WAYPOINT_FACTORY_HPP

#include "Engine/Waypoint/Waypoint.hpp"

class RasterTerrain;

/**
 * This class can create new Waypoint instances and can help fill out
 * some attributes.  It is meant to encapsulate the initialisation of
 * attributes that are not specific to the #WaypointReaderBase
 * implementation.
 */
class WaypointFactory {
  WaypointOrigin origin;
  const RasterTerrain *terrain;

public:
  explicit WaypointFactory(WaypointOrigin _origin,
                           const RasterTerrain *_terrain=nullptr)
    :origin(_origin), terrain(_terrain) {}

  Waypoint Create(const GeoPoint &location) const {
    Waypoint w(location);
    w.origin = origin;
    w.original_id = 0;
    return w;
  }

  /**
   * We don't know the elevation of this waypoint, and this method
   * shall find a viable fallback (e.g. by looking up the terrain
   * elevation at the given location).
   *
   * @return true if a fallback was found and Waypoint::elevation was
   * set, false if no fallback was found
   */
  bool FallbackElevation(Waypoint &waypoint) const;
};

#endif
