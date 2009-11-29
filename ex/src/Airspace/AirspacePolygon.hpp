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
#ifndef AIRSPACEPOLYGON_HPP
#define AIRSPACEPOLYGON_HPP

#include "AbstractAirspace.hpp"
#include "Navigation/SearchPointVector.hpp"
#include <vector>

/**
 * General polygon form airspace
 *
 */
class AirspacePolygon: public AbstractAirspace 
{
public:
  /** 
   * Constructor.  For testing, pts vector is a cloud of points,
   * and the airspace polygon becomes the convex hull border.
   * \todo remove convex hack for testing
   *
   * @param pts Vector representing border
   *
   * @return Initialised airspace object
   */
  AirspacePolygon(const std::vector<GEOPOINT>& pts);

  /** 
   * Compute bounding box enclosing the airspace.  Rounds up/down
   * so discretisation ensures bounding box is indeed enclosing.
   * 
   * @param task_projection Projection used for flat-earth representation
   * 
   * @return Enclosing bounding box
   */
  const FlatBoundingBox get_bounding_box(const TaskProjection& task_projection);

/** 
 * Get arbitrary center or reference point for use in determining
 * overall center location of all airspaces
 * 
 * @return Location of reference point
 */
  const GEOPOINT get_center();

  /** 
   * Checks whether an aircraft is inside the airspace.
   * This is slow because it uses geodesic calculations
   * 
   * @param loc State about which to test inclusion
   * 
   * @return true if aircraft is inside airspace boundary
   */
  bool inside(const AIRCRAFT_STATE &loc) const;

  /** 
   * Checks whether a line intersects with the airspace.
   * Can be approximate by using flat-earth representation internally.
   * 
   * @param g1 Location of origin of search vector
   * @param vec Line from origin
   * @param tp Projection used by flat-earth representation
   * 
   * @return true if the line intersects the airspace
   */
  bool intersects(const GEOPOINT& g1, 
                  const GeoVector &vec,
                  const TaskProjection& tp) const;

private:
  SearchPointVector border;

/** 
 * Project border.
 */
  virtual void project(const TaskProjection& tp);

public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AirspacePolygon& as);
#endif
  DEFINE_VISITABLE()
};

#endif
