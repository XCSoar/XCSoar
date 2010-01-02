/* Copyright_License {

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
#ifndef AIRSPACE_HPP
#define AIRSPACE_HPP

#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "AbstractAirspace.hpp"
#include "Util/GenericVisitor.hpp"

class AtmosphericPressure;

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
  public FlatBoundingBox,
  public BaseVisitable<>
{
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
  Airspace(const GEOPOINT&loc, const TaskProjection& task_projection, const
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
  Airspace(const GEOPOINT &ll, 
           const GEOPOINT &ur,
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
  bool inside(const AIRCRAFT_STATE &loc) const;

  /** 
   * Checks whether a point is inside the airspace lateral boundary. 
   *
   * @param loc Location to check for enclosure
   * 
   * @return true if location is inside airspace
   */
  bool inside(const GEOPOINT &loc) const;

  /** 
   * Checks whether a flat-earth ray intersects with the airspace
   * bounding box.
   * 
   * @param ray Flat-earth ray to check for intersection
   * 
   * @return true if ray intersects or wholly enclosed by airspace
   */
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
  AirspaceIntersectionVector intersects(const GEOPOINT& g1, const GeoVector &vec) const;

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

private:

  mutable AbstractAirspace *pimpl_airspace;

public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const Airspace& ts);
#endif
  
  /** 
   * Accepts a visitor and directs it to the contained concrete airspace.
   * 
   * @param v Visitor to accept
   */
  void Accept(BaseVisitor &v) const;
};

#endif
