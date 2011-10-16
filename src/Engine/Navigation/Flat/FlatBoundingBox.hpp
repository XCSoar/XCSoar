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
#ifndef FLATBOUNDINGBOX_HPP
#define FLATBOUNDINGBOX_HPP

#include "FlatGeoPoint.hpp"
#include "FlatRay.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Navigation/Geometry/BoundingBoxDistance.hpp"
#include "Compiler.h"

/**
 * Structure defining 2-d integer projected coordinates defining
 * a lower left and upper right bounding box.
 * For use in kd-tree storage of 2-d objects.
 */
class FlatBoundingBox
{
  FlatGeoPoint bb_ll;
  FlatGeoPoint bb_ur;

public:
  friend class TaskProjection;

  /** Non-initialising constructor. */
  FlatBoundingBox() {}

  /**
   * Constructor given bounds
   *
   * @param ll Lower left location
   * @param ur Upper right location
   */
  gcc_constexpr_ctor
  FlatBoundingBox(const FlatGeoPoint ll, const FlatGeoPoint ur)
    :bb_ll(ll.Longitude,ll.Latitude),
     bb_ur(ur.Longitude,ur.Latitude) {}

  /**
   * Constructor given center point and radius
   * (produces a box enclosing a circle of given radius at center point)
   *
   * @param loc Location of center point
   * @param range Radius in projected units
   */
  gcc_constexpr_ctor
  FlatBoundingBox(const FlatGeoPoint loc, const unsigned range = 0)
    :bb_ll(loc.Longitude - range, loc.Latitude - range),
     bb_ur(loc.Longitude + range, loc.Latitude + range) {}

  /**
   * Calculate non-overlapping distance from one box to another.
   *
   * @param f That box
   *
   * @return Distance in projected units (or zero if overlapping)
   */
  gcc_pure
  unsigned Distance(const FlatBoundingBox &f) const;

  /**
   * Test whether a point is inside the bounding box
   *
   * @param loc Point to test
   *
   * @return true if loc is inside the bounding box
   */
  gcc_pure
  bool IsInside(const FlatGeoPoint& loc) const;

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
        return d.bb_ll.Longitude;
      case 1:
        return d.bb_ll.Latitude;
      case 2:
        return d.bb_ur.Longitude;
      case 3:
        return d.bb_ur.Latitude;
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
      return BBDist(dim, max((dim < 2) ? (b - a) : (a - b), 0));
    }
  };

  /**
   * Test ray-box intersection
   *
   * @param ray Ray to test for intersection
   *
   * @return True if ray intersects with this bounding box
   */
  gcc_pure
  bool Intersects(const FlatRay& ray) const;

  /**
   * Get center of bounding box
   *
   * @return Center in flat coordinates
   */
  gcc_pure
  FlatGeoPoint GetCenter() const;

  /**
   * Determine whether these bounding boxes overlap
   */
  gcc_pure
  bool Overlaps(const FlatBoundingBox& other) const;

  /**
   * Expand the bounding box to include this point
   */
  void Expand(const FlatGeoPoint& p) {
    bb_ll.Longitude = std::min(bb_ll.Longitude, p.Longitude);
    bb_ur.Longitude = std::max(bb_ur.Longitude, p.Longitude);
    bb_ll.Latitude = std::min(bb_ll.Latitude, p.Latitude);
    bb_ur.Latitude = std::max(bb_ur.Latitude, p.Latitude);
  }

  /**
   * Expand the bounding box to include this bounding box
   */
  void Merge(const FlatBoundingBox& p) {
    bb_ll.Longitude = std::min(bb_ll.Longitude, p.bb_ll.Longitude);
    bb_ur.Longitude = std::max(bb_ur.Longitude, p.bb_ur.Longitude);
    bb_ll.Latitude = std::min(bb_ll.Latitude, p.bb_ll.Latitude);
    bb_ur.Latitude = std::max(bb_ur.Latitude, p.bb_ur.Latitude);
  }

  /**
   * Shift the bounding box by an offset p
   */
  void Shift(const FlatGeoPoint &offset) {
    bb_ll = bb_ll + offset;
    bb_ur = bb_ur + offset;
  }

  /**
   * Expand the border by x amount
   */
  void ExpandByOne() {
    bb_ll.Longitude--;
    bb_ur.Longitude++;
    bb_ll.Latitude--;
    bb_ur.Latitude++;
  }
};

#endif
