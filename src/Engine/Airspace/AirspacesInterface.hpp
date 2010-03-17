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
#ifndef AIRSPACESINTERFACE_HPP
#define AIRSPACESINTERFACE_HPP

#include "Airspace.hpp"
#include "AirspacePredicate.hpp"
#include "Navigation/TaskProjection.hpp"
#include <kdtree++/kdtree.hpp>

class AirspaceVisitor;
class AirspaceIntersectionVisitor;


class AirspacesInterface {
public:
  typedef std::vector<Airspace> AirspaceVector; /**< Vector of airspaces (used internally) */

  /**
   * Type of KD-tree data structure for airspace container
   */
  typedef KDTree::KDTree<4, 
                         Airspace, 
                         FlatBoundingBox::kd_get_bounds,
                         FlatBoundingBox::kd_distance
                         > AirspaceTree;


  /** 
   * Search for airspaces within range of the aircraft.
   * 
   * @param location location of aircraft, from which to search
   * @param range distance in meters of search radius
   * @param condition condition to be applied to matches
   * 
   * @return vector of airspaces intersecting search radius
   */
  virtual const AirspaceVector scan_range(const GEOPOINT location,
                                          const fixed range,
                                          const AirspacePredicate &condition
                                          =AirspacePredicate::always_true) const = 0;

  virtual void lock() const = 0;
  virtual void unlock() const = 0;
  virtual unsigned size() const = 0;
  virtual AirspaceTree::const_iterator begin() const = 0;
  virtual AirspaceTree::const_iterator end() const = 0;
};

#endif
