/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#ifndef AIRSPACESINTERFACE_HPP
#define AIRSPACESINTERFACE_HPP

#include "Util/SliceAllocator.hpp"
#include "Airspace.hpp"
#include "Geo/Flat/BoundingBoxDistance.hpp"

#include <kdtree++/kdtree.hpp>

/**
 * Abstract class for interface to #Airspaces database.
 * Allows references to the #Airspaces to be substituted for a
 * facade protected class where locking is required.
 */
class AirspacesInterface {
  /** Function object used by kd-tree to index coordinates */
  struct kd_get_bounds
  {
    /** Used by kd-tree */
    typedef int result_type;

    /**
     * Retrieve coordinate value given coordinate index and object
     *
     * @param d Object being stored in kd-tree
     * @param k Index of coordinate
     *
     * @return Coordinate value
     */
    int operator() ( const FlatBoundingBox &d, const unsigned k) const {
      switch(k) {
      case 0:
        return d.GetLowerLeft().longitude;
      case 1:
        return d.GetLowerLeft().latitude;
      case 2:
        return d.GetUpperRight().longitude;
      case 3:
        return d.GetUpperRight().latitude;
      };
      return 0;
    };
  };

  /**
   * Distance metric function object used by kd-tree.  This specialisation
   * allows for overlap; distance is zero with overlap, otherwise the minimum
   * distance between two regions.
   */
  struct kd_distance
  {
    /** Distance operator for overlap functionality */
    typedef BBDist distance_type;

    /**
     * \todo document this!
     *
     * @param a
     * @param b
     * @param dim
     *
     * @return Distance on axis
     */
    distance_type operator()(const int a, const int b, const size_t dim) const {
      return BBDist(dim, std::max((dim < 2) ? (b - a) : (a - b), 0));
    }
  };

public:
  typedef std::vector<Airspace> AirspaceVector; /**< Vector of airspaces (used internally) */

  /**
   * Type of KD-tree data structure for airspace container
   */
  typedef KDTree::KDTree<4, 
                         Airspace, 
                         kd_get_bounds, kd_distance,
                         std::less<kd_get_bounds::result_type>,
                         SliceAllocator<KDTree::_Node<Airspace>, 256>
                         > AirspaceTree;
};

#endif
