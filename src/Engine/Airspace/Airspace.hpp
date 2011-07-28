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
#ifndef AIRSPACE_HPP
#define AIRSPACE_HPP

#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Compiler.h"

#ifdef DO_PRINT
#include <iostream>
#endif

struct AircraftState;
class AtmosphericPressure;
class AbstractAirspace;
class AirspaceActivity;

#include <vector>
typedef std::vector< std::pair<GeoPoint,GeoPoint> > AirspaceIntersectionVector;

/**
 * Single object container for actual airspaces, to be stored in Airspaces object
 * This class manages the bounding box of the actual airspace.
 *
 * This follows envelope-letter
 * idiom, in which the AbstractAirspace is the letter and this class
 * Airspace is an envelope, containing bounding box information for
 * use with high performance search structures.
 * 
 */
class Airspace: 
  public FlatBoundingBox
{
  mutable AbstractAirspace *pimpl_airspace;

public:

  /** 
   * Constructor for actual airspaces.  
   *
   * @param airspace actual concrete airspace to create an envelope for
   * @param tp task projection to be used for flat-earth representation
   * 
   * @return airspace letter inside envelope suitable for insertion in a search structure
   */
  Airspace(AbstractAirspace& airspace,
           const TaskProjection& tp);

  /** 
   * Constructor for virtual airspaces for use in range-based
   * intersection queries
   * 
   * @param loc Location about which to create a virtual airspace envelope
   * @param task_projection projection to be used for flat-earth representation
   * @param range range in meters of virtual bounding box
   * 
   * @return dummy airspace envelope
   */
  Airspace(const GeoPoint&loc, const TaskProjection& task_projection, const
    fixed range=fixed_zero):
    FlatBoundingBox(task_projection.project(loc),
                    task_projection.project_range(loc,range)),
    pimpl_airspace(NULL)
  {
  };

  /** 
   * Constructor for virtual airspaces for use in bounding-box
   * specified intersection queries
   * 
   * @param ll Lower left corner of bounding box
   * @param ur Upper right corner of bounding box
   * @param task_projection projection to be used for flat-earth representation
   * 
   * @return dummy airspace envelope
   */
  Airspace(const GeoPoint &ll, 
           const GeoPoint &ur,
           const TaskProjection& task_projection):
    FlatBoundingBox(task_projection.project(ll),
                    task_projection.project(ur)), 
    pimpl_airspace(NULL)
  {
  };

  /** 
   * Checks whether an aircraft is inside the airspace. 
   *
   * @param loc Location to check for enclosure
   * 
   * @return true if aircraft is inside airspace
   */
  gcc_pure
  bool inside(const AircraftState &loc) const;

  /** 
   * Checks whether a point is inside the airspace lateral boundary. 
   *
   * @param loc Location to check for enclosure
   * 
   * @return true if location is inside airspace
   */
  gcc_pure
  bool inside(const GeoPoint &loc) const;

  /** 
   * Checks whether a flat-earth ray intersects with the airspace
   * bounding box.
   * 
   * @param ray Flat-earth ray to check for intersection
   * 
   * @return true if ray intersects or wholly enclosed by airspace
   */
  gcc_pure
  bool intersects(const FlatRay& ray) const;

  /** 
   * Checks whether a line intersects with the airspace, by directing
   * the query to the enclosed concrete airspace.
   * 
   * @param g1 Location of origin of search vector
   * @param vec Line from origin
   * 
   * @return true if the line intersects the airspace
   */
  gcc_pure
  AirspaceIntersectionVector intersects(const GeoPoint& g1, const GeoVector &vec) const;

  /** 
   * Destroys concrete airspace enclosed by this instance if present.
   * Note that this should not be called by clients but only by the
   * master store.  Many copies of this airspace may point to the same
   * concrete airspace so have to be careful here.
   * 
   */
  void destroy();

/** 
 * Accessor for contained AbstractAirspace 
 * 
 * @return Airspace letter
 */
  AbstractAirspace *get_airspace() const {
    return pimpl_airspace;
  };

  /** 
   * Set terrain altitude for AGL-referenced airspace altitudes 
   * 
   * @param alt Height above MSL of terrain (m) at center
   */
  void set_ground_level(const fixed alt) const;

  /** 
   * Set QNH pressure for FL-referenced airspace altitudes 
   * 
   * @param press Atmospheric pressure model and QNH
   */
  void set_flight_level(const AtmosphericPressure &press) const;

  /**
   * Set activity based on day mask
   *
   * @param days Mask of activity
   */
  void set_activity(const AirspaceActivity mask) const;

  /**
   * Clear the convex clearance polygon
   */
  void clear_clearance() const;

  /**
   * Equality operator, matches if contained airspace is the same
   */
  bool operator==(Airspace const& a) const {
    return (get_airspace() == a.get_airspace());
  }

public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const Airspace& ts);
#endif
};

#endif
