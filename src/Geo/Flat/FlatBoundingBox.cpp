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

#include "FlatBoundingBox.hpp"
#include "FlatRay.hpp"
#include "Math/FastMath.hpp"
#include "Math/Util.hpp"

#include <algorithm>

static constexpr inline unsigned
Distance1D(int a1, int a2, int b1, int b2)
{
  return a2 < b1
    ? b1 - a2 /* A is left of B */
    : (a1 >= b2
       ? a1 - b2 /* A is right of B */
       : 0); /* A and B overlap */
}

unsigned
FlatBoundingBox::SquareDistanceTo(FlatGeoPoint p) const
{
  if (IsInside(p))
    return 0;

  unsigned dx = Distance1D(lower_left.x, upper_right.x, p.x, p.x);
  unsigned dy = Distance1D(lower_left.y, upper_right.y, p.y, p.y);
  return Square(dx) + Square(dy);
}

unsigned
FlatBoundingBox::Distance(const FlatBoundingBox &f) const
{
  unsigned dx = Distance1D(lower_left.x, upper_right.x,
                           f.lower_left.x, f.upper_right.x);
  unsigned dy = Distance1D(lower_left.y, upper_right.y,
                           f.lower_left.y, f.upper_right.y);

  return ihypot(dx, dy);
}

bool
FlatBoundingBox::Intersects(const FlatRay& ray) const
{
  double tmin = 0, tmax = 1;

  // X
  if (ray.vector.x == 0) {
    // ray is parallel to slab. No hit if origin not within slab
    if (ray.point.x < lower_left.x || ray.point.x > upper_right.x)
      return false;
  } else {
    // compute intersection t value of ray with near/far plane of slab
    auto t1 = (lower_left.x - ray.point.x) * ray.fx;
    auto t2 = (upper_right.x - ray.point.x) * ray.fx;
    // make t1 be intersection with near plane, t2 with far plane
    if (t1 > t2)
      std::swap(t1, t2);

    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    // exit with no collision as soon as slab intersection becomes empty
    if (tmin > tmax)
      return false;
  }

  // Y
  if (ray.vector.y == 0) {
    // ray is parallel to slab. No hit if origin not within slab
    if (ray.point.y < lower_left.y || ray.point.y > upper_right.y)
      return false;
  } else {
    // compute intersection t value of ray with near/far plane of slab
    auto t1 = (lower_left.y - ray.point.y) * ray.fy;
    auto t2 = (upper_right.y - ray.point.y) * ray.fy;
    // make t1 be intersection with near plane, t2 with far plane
    if (t1 > t2)
      std::swap(t1, t2);

    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    // exit with no collision as soon as slab intersection becomes empty
    if (tmin > tmax)
      return false;
  }
  return true;
}

FlatGeoPoint
FlatBoundingBox::GetCenter() const
{
  /// @todo This will break if overlaps 360/0
  return FlatGeoPoint((lower_left.x + upper_right.x) / 2,
                      (lower_left.y + upper_right.y) / 2);
}

bool
FlatBoundingBox::Overlaps(const FlatBoundingBox& other) const
{
  if (lower_left.x > other.upper_right.x)
    return false;
  if (upper_right.x < other.lower_left.x)
    return false;
  if (lower_left.y > other.upper_right.y)
    return false;
  if (upper_right.y < other.lower_left.y)
    return false;

  return true;
}

bool
FlatBoundingBox::IsInside(const FlatGeoPoint& loc) const
{
  if (loc.x < lower_left.x)
    return false;
  if (loc.x > upper_right.x)
    return false;
  if (loc.y < lower_left.y)
    return false;
  if (loc.y > upper_right.y)
    return false;

  return true;
}
