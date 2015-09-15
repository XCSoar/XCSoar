/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Compiler.h"

#include <algorithm>

class FlatRay;

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
  /** Non-initialising constructor. */
  FlatBoundingBox() = default;

  /**
   * Constructor given bounds
   *
   * @param ll Lower left location
   * @param ur Upper right location
   */
  constexpr
  FlatBoundingBox(const FlatGeoPoint ll, const FlatGeoPoint ur)
    :bb_ll(ll.x, ll.y), bb_ur(ur.x, ur.y) {}

  /**
   * Constructor given center point and radius
   * (produces a box enclosing a circle of given radius at center point)
   *
   * @param loc Location of center point
   * @param range Radius in projected units
   */
  constexpr
  FlatBoundingBox(const FlatGeoPoint loc, const unsigned range = 0)
    :bb_ll(loc.x - range, loc.y - range),
    bb_ur(loc.x + range, loc.y + range) {}

  constexpr const FlatGeoPoint &GetLowerLeft() const {
    return bb_ll;
  }

  constexpr const FlatGeoPoint &GetUpperRight() const {
    return bb_ur;
  }

  constexpr int GetLeft() const {
    return bb_ll.x;
  }

  constexpr int GetTop() const {
    return bb_ur.y;
  }

  constexpr int GetRight() const {
    return bb_ur.x;
  }

  constexpr int GetBottom() const {
    return bb_ll.y;
  }

  constexpr FlatGeoPoint GetTopLeft() const {
    return FlatGeoPoint(GetLeft(), GetTop());
  }

  constexpr FlatGeoPoint GetBottomRight() const {
    return FlatGeoPoint(GetRight(), GetBottom());
  }

  constexpr unsigned GetWidth() const {
    return GetRight() - GetLeft();
  }

  constexpr unsigned GetHeight() const {
    return GetTop() - GetBottom();
  }

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
    bb_ll.x = std::min(bb_ll.x, p.x);
    bb_ur.x = std::max(bb_ur.x, p.x);
    bb_ll.y = std::min(bb_ll.y, p.y);
    bb_ur.y = std::max(bb_ur.y, p.y);
  }

  /**
   * Expand the bounding box to include this bounding box
   */
  void Merge(const FlatBoundingBox& p) {
    bb_ll.x = std::min(bb_ll.x, p.bb_ll.x);
    bb_ur.x = std::max(bb_ur.x, p.bb_ur.x);
    bb_ll.y = std::min(bb_ll.y, p.bb_ll.y);
    bb_ur.y = std::max(bb_ur.y, p.bb_ur.y);
  }

  /**
   * Shift the bounding box by an offset p
   */
  void Shift(const FlatGeoPoint &offset) {
    bb_ll = bb_ll + offset;
    bb_ur = bb_ur + offset;
  }

  FlatBoundingBox &Grow(int delta) {
    bb_ll.x -= delta;
    bb_ll.y -= delta;
    bb_ur.x += delta;
    bb_ur.y += delta;
    return *this;
  }

  /**
   * Expand the border by x amount
   */
  void ExpandByOne() {
    --bb_ll.x;
    ++bb_ur.x;
    --bb_ll.y;
    ++bb_ur.y;
  }
};

#endif
