/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include <algorithm>

class FlatRay;

template<typename I>
concept FlatGeoPointIterator = requires (I a, I b) {
  a != b;
  ++a;
  static_cast<FlatGeoPoint>(*a);
};

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
  FlatBoundingBox() noexcept = default;

  /**
   * Constructor given bounds
   *
   * @param ll Lower left location
   * @param ur Upper right location
   */
  constexpr FlatBoundingBox(FlatGeoPoint ll, FlatGeoPoint ur) noexcept
    :lower_left(ll), upper_right(ur) {}

  /**
   * Constructor given center point and radius
   * (produces a box enclosing a circle of given radius at center point)
   *
   * @param loc Location of center point
   * @param range Radius in projected units
   */
  constexpr FlatBoundingBox(FlatGeoPoint loc, unsigned range=0) noexcept
    :lower_left(loc.x - range, loc.y - range),
     upper_right(loc.x + range, loc.y + range) {}

  /**
   * Calculate the bounding box of a non-empty range of FlatGeoPoints
   * specified by two (non-equal) iterators.
   */
  template<FlatGeoPointIterator I>
  constexpr FlatBoundingBox(I begin, I end) noexcept
    :lower_left(*begin), upper_right(*begin)
  {
    for (auto i = std::next(begin); i != end; ++i)
      Expand(*i);
  }

  constexpr const FlatGeoPoint &GetLowerLeft() const noexcept {
    return lower_left;
  }

  constexpr const FlatGeoPoint &GetUpperRight() const noexcept {
    return upper_right;
  }

  constexpr int GetLeft() const noexcept {
    return lower_left.x;
  }

  constexpr int GetTop() const noexcept {
    return upper_right.y;
  }

  constexpr int GetRight() const noexcept {
    return upper_right.x;
  }

  constexpr int GetBottom() const noexcept {
    return lower_left.y;
  }

  constexpr FlatGeoPoint GetTopLeft() const noexcept {
    return FlatGeoPoint(GetLeft(), GetTop());
  }

  constexpr FlatGeoPoint GetBottomRight() const noexcept {
    return FlatGeoPoint(GetRight(), GetBottom());
  }

  constexpr unsigned GetWidth() const noexcept {
    return GetRight() - GetLeft();
  }

  constexpr unsigned GetHeight() const noexcept {
    return GetTop() - GetBottom();
  }

  [[gnu::pure]]
  unsigned SquareDistanceTo(FlatGeoPoint p) const noexcept;

  /**
   * Calculate non-overlapping distance from one box to another.
   *
   * @param f That box
   *
   * @return Distance in projected units (or zero if overlapping)
   */
  [[gnu::pure]]
  unsigned Distance(const FlatBoundingBox &f) const noexcept;

  /**
   * Test whether a point is inside the bounding box
   *
   * @param loc Point to test
   *
   * @return true if loc is inside the bounding box
   */
  [[gnu::pure]]
  bool IsInside(const FlatGeoPoint &loc) const noexcept;

  /**
   * Test ray-box intersection
   *
   * @param ray Ray to test for intersection
   *
   * @return True if ray intersects with this bounding box
   */
  [[gnu::pure]]
  bool Intersects(const FlatRay &ray) const noexcept;

  /**
   * Get center of bounding box
   *
   * @return Center in flat coordinates
   */
  [[gnu::pure]]
  FlatGeoPoint GetCenter() const noexcept;

  /**
   * Determine whether these bounding boxes overlap
   */
  [[gnu::pure]]
  bool Overlaps(const FlatBoundingBox& other) const noexcept;

  /**
   * Expand the bounding box to include this point
   */
  constexpr void Expand(const FlatGeoPoint &p) noexcept {
    lower_left.x = std::min(lower_left.x, p.x);
    upper_right.x = std::max(upper_right.x, p.x);
    lower_left.y = std::min(lower_left.y, p.y);
    upper_right.y = std::max(upper_right.y, p.y);
  }

  /**
   * Expand the bounding box to include this bounding box
   */
  void Merge(const FlatBoundingBox &p) noexcept {
    lower_left.x = std::min(lower_left.x, p.lower_left.x);
    upper_right.x = std::max(upper_right.x, p.upper_right.x);
    lower_left.y = std::min(lower_left.y, p.lower_left.y);
    upper_right.y = std::max(upper_right.y, p.upper_right.y);
  }

  /**
   * Shift the bounding box by an offset p
   */
  void Shift(const FlatGeoPoint &offset) noexcept {
    lower_left = lower_left + offset;
    upper_right = upper_right + offset;
  }

  FlatBoundingBox &Grow(int delta) noexcept {
    lower_left.x -= delta;
    lower_left.y -= delta;
    upper_right.x += delta;
    upper_right.y += delta;
    return *this;
  }

  /**
   * Expand the border by x amount
   */
  void ExpandByOne() noexcept {
    --lower_left.x;
    ++upper_right.x;
    --lower_left.y;
    ++upper_right.y;
  }
};

#endif
