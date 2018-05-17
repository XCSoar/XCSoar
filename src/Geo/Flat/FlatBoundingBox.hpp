/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
struct FlatBoundingBox
{
  FlatGeoPoint lower_left;
  FlatGeoPoint upper_right;

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
    :lower_left(ll.x, ll.y), upper_right(ur.x, ur.y) {}

  /**
   * Constructor given center point and radius
   * (produces a box enclosing a circle of given radius at center point)
   *
   * @param loc Location of center point
   * @param range Radius in projected units
   */
  constexpr
  FlatBoundingBox(const FlatGeoPoint loc, const unsigned range = 0)
    :lower_left(loc.x - range, loc.y - range),
    upper_right(loc.x + range, loc.y + range) {}

  constexpr const FlatGeoPoint &GetLowerLeft() const {
    return lower_left;
  }

  constexpr const FlatGeoPoint &GetUpperRight() const {
    return upper_right;
  }

  constexpr int GetLeft() const {
    return lower_left.x;
  }

  constexpr int GetTop() const {
    return upper_right.y;
  }

  constexpr int GetRight() const {
    return upper_right.x;
  }

  constexpr int GetBottom() const {
    return lower_left.y;
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

  gcc_pure
  unsigned SquareDistanceTo(FlatGeoPoint p) const;

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
    lower_left.x = std::min(lower_left.x, p.x);
    upper_right.x = std::max(upper_right.x, p.x);
    lower_left.y = std::min(lower_left.y, p.y);
    upper_right.y = std::max(upper_right.y, p.y);
  }

  /**
   * Expand the bounding box to include this bounding box
   */
  void Merge(const FlatBoundingBox& p) {
    lower_left.x = std::min(lower_left.x, p.lower_left.x);
    upper_right.x = std::max(upper_right.x, p.upper_right.x);
    lower_left.y = std::min(lower_left.y, p.lower_left.y);
    upper_right.y = std::max(upper_right.y, p.upper_right.y);
  }

  /**
   * Shift the bounding box by an offset p
   */
  void Shift(const FlatGeoPoint &offset) {
    lower_left = lower_left + offset;
    upper_right = upper_right + offset;
  }

  FlatBoundingBox &Grow(int delta) {
    lower_left.x -= delta;
    lower_left.y -= delta;
    upper_right.x += delta;
    upper_right.y += delta;
    return *this;
  }

  /**
   * Expand the border by x amount
   */
  void ExpandByOne() {
    --lower_left.x;
    ++upper_right.x;
    --lower_left.y;
    ++upper_right.y;
  }
};

#endif
