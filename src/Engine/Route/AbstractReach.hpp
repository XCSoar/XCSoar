/* Copyright_License {

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

#ifndef ABSTRACTREACH_HPP
#define ABSTRACTREACH_HPP

#include "Navigation/Flat/FlatGeoPoint.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Navigation/TaskProjection.hpp"

class RoutePolars;
class RasterMap;

class AbstractReach {
  friend class PrintHelper;
protected:
  TaskProjection task_proj;
public:
  AbstractReach() {};

  /**
   * Reset as if never called
   */
  virtual void reset() {};

  /**
   * Solve for straight line and turning reach footprint.
   * Note that this class applies terrain safety height from the
   * rpolars configuration.
   *
   * @param origin 3d location to check
   * @param rpolars RoutePolars performance model
   * @param terrain Terrain map of obstacles
   *
   * @return True if solution successful
   */
  virtual bool solve(const AGeoPoint origin,
                     const RoutePolars &rpolars,
                     const RasterMap* terrain);

  /**
   * Determine whether a point is within the reach.
   * Requires solve() to have been called for positive results.
   * If turning solution has not been obtained, reverts to straight-line.
   *
   * @param origin Location to check
   * @param turning Whether to allow turning reach or straight-line
   *
   * @return true if point is inside reach polygon
   */
  virtual bool is_inside(const GeoPoint origin, const bool turning=true) const = 0;

  /**
   * Find arrival height at destination.
   *
   * Requires solve() to have been called for positive results.
   *
   * @param dest Destination location
   * @param rpolars RoutePolars performance model
   * @param arrival_height height at arrival or -1 if out of reach
   *
   * @return true if check was successful
   */
  virtual bool find_positive_arrival(const AGeoPoint dest,
                                     const RoutePolars &rpolars,
                                     short& arrival_height) const = 0;
  /**
   * Find base terrain height in reachable area.
   * 
   * Requires solve() to have been called for positive results.
   *
   * @return height (m) of terrain base
   */
  virtual short get_terrain_base() const = 0;
};

#endif
