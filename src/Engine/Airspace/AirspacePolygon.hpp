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
#ifndef AIRSPACEPOLYGON_HPP
#define AIRSPACEPOLYGON_HPP

#include "AbstractAirspace.hpp"
#include <vector>

#ifdef DO_PRINT
#include <iostream>
#endif

/**
 * General polygon form airspace
 *
 */
class AirspacePolygon: 
  public AbstractAirspace 
{
public:
  /** 
   * Constructor.  For testing, pts vector is a cloud of points,
   * and the airspace polygon becomes the convex hull border.
   *
   * @param pts Vector representing border
   * @param prune If true, converts border to convex hull of points (for testing only)
   *
   * @return Initialised airspace object
   */
  AirspacePolygon(const std::vector<GeoPoint>& pts,
    const bool prune = false);

/** 
 * Get arbitrary center or reference point for use in determining
 * overall center location of all airspaces
 * 
 * @return Location of reference point
 */
  const GeoPoint GetCenter() const;

  /** 
   * Checks whether an aircraft is inside the airspace.
   * This is slow because it uses geodesic calculations
   * 
   * @param loc State about which to test inclusion
   * 
   * @return true if aircraft is inside airspace boundary
   */
  bool Inside(const GeoPoint &loc) const;

  /** 
   * Checks whether a line intersects with the airspace.
   * Can be approximate by using flat-earth representation internally.
   * 
   * @param g1 Location of origin of search vector
   * @param vec Line from origin
   * 
   * @return Vector of intersection pairs if the line intersects the airspace
   */
  AirspaceIntersectionVector Intersects(const GeoPoint& g1, 
                                        const GeoVector &vec) const;

  GeoPoint ClosestPoint(const GeoPoint& loc) const;

public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AirspacePolygon& as);
#endif
};

#endif
