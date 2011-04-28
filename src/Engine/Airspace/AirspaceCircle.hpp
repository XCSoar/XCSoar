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

#ifndef AIRSPACECIRCLE_HPP
#define AIRSPACECIRCLE_HPP

#include "AbstractAirspace.hpp"

#ifdef DO_PRINT
#include <iostream>
#endif

/**
 * Airspace object defined by the area within a distance (radius) from a center point 
 */
class AirspaceCircle: 
    public AbstractAirspace 
{
  const GeoPoint m_center;
  const fixed m_radius;

public:

  /** 
   * Constructor
   * 
   * @param loc Center point of circle
   * @param _radius Radius in meters of airspace boundary
   * 
   * @return Initialised airspace object
   */
  AirspaceCircle(const GeoPoint &loc, const fixed _radius);

  /**
   * Get arbitrary center or reference point for use in determining
   * overall center location of all airspaces
   *
   * @return Location of reference point
   */
  const GeoPoint get_center() const {
    return m_center;
  }

  /** 
   * Checks whether an aircraft is inside the airspace.
   * This is slow because it uses geodesic calculations
   * 
   * @param loc State about which to test inclusion
   * 
   * @return true if aircraft is inside airspace boundary
   */
  bool inside(const GeoPoint &loc) const;

  /** 
   * Checks whether a line intersects with the airspace.
   * Can be approximate by using flat-earth representation internally.
   * 
   * @param g1 Location of origin of search vector
   * @param vec Line from origin
   * 
   * @return Vector of intersection pairs if the line intersects the airspace
   */
  AirspaceIntersectionVector intersects(const GeoPoint& g1, 
                                        const GeoVector &vec) const;

  /**
   * Returns the GeoPoint inside the AirspaceCircle, that is closest
   * to the given GeoPoint loc
   *
   * @param loc GeoPoint that should be calculated with
   *
   * @return Closest GeoPoint in the AirspaceCircle
   */
  GeoPoint closest_point(const GeoPoint& loc) const;

  /**
   * Accessor for radius
   *
   * @return Radius of circle (m)
   */
  const fixed& get_radius() const {
    return m_radius;
  }

public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AirspaceCircle& as);
#endif
};


#endif
