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

#include "FlatBoundingBox.hpp"

#include "Math/fixed.hpp"
#include "Math/FastMath.h"

#include <algorithm>

unsigned
FlatBoundingBox::distance(const FlatBoundingBox &f) const
{
  if (overlaps(f))
    return 0;

  long dx = max(0, min(f.bb_ll.Longitude - bb_ur.Longitude,
                       bb_ll.Longitude - f.bb_ur.Longitude));
  long dy = max(0, min(f.bb_ll.Latitude - bb_ur.Latitude,
                       bb_ll.Latitude - f.bb_ur.Latitude));

  return lhypot(dx, dy);
}

bool
FlatBoundingBox::intersects(const FlatRay& ray) const
{
  fixed tmin = fixed_zero;
  fixed tmax = fixed_one;
  
  // Longitude
  if (ray.vector.Longitude == 0) {
    // ray is parallel to slab. No hit if origin not within slab
    if (ray.point.Longitude < bb_ll.Longitude ||
        ray.point.Longitude > bb_ur.Longitude)
      return false;
  } else {
    // compute intersection t value of ray with near/far plane of slab
    fixed t1 = (bb_ll.Longitude - ray.point.Longitude) * ray.fx;
    fixed t2 = (bb_ur.Longitude - ray.point.Longitude) * ray.fx;
    // make t1 be intersection with near plane, t2 with far plane
    if (t1 > t2)
      std::swap(t1, t2);

    tmin = max(tmin, t1);
    tmax = min(tmax, t2);
    // exit with no collision as soon as slab intersection becomes empty
    if (tmin > tmax)
      return false;
  }

  // Latitude
  // Longitude
  if (ray.vector.Latitude == 0) {
    // ray is parallel to slab. No hit if origin not within slab
    if (ray.point.Latitude < bb_ll.Latitude ||
        ray.point.Latitude > bb_ur.Latitude)
      return false;
  } else {
    // compute intersection t value of ray with near/far plane of slab
    fixed t1 = (bb_ll.Latitude - ray.point.Latitude) * ray.fy;
    fixed t2 = (bb_ur.Latitude - ray.point.Latitude) * ray.fy;
    // make t1 be intersection with near plane, t2 with far plane
    if (t1 > t2)
      std::swap(t1, t2);

    tmin = max(tmin, t1);
    tmax = min(tmax, t2);
    // exit with no collision as soon as slab intersection becomes empty
    if (tmin > tmax)
      return false;
  }
  return true;
}

FlatGeoPoint
FlatBoundingBox::get_center() const
{
  /// @todo This will break if overlaps 360/0
  return FlatGeoPoint((bb_ll.Longitude + bb_ur.Longitude) / 2,
                      (bb_ll.Latitude + bb_ur.Latitude) / 2);
}

bool
FlatBoundingBox::overlaps(const FlatBoundingBox& other) const
{
  if (bb_ll.Longitude > other.bb_ur.Longitude)
    return false;
  if (bb_ur.Longitude < other.bb_ll.Longitude)
    return false;
  if (bb_ll.Latitude > other.bb_ur.Latitude)
    return false;
  if (bb_ur.Latitude < other.bb_ll.Latitude)
    return false;

  return true;
}

bool
FlatBoundingBox::is_inside(const FlatGeoPoint& loc) const
{
  if (loc.Longitude < bb_ll.Longitude)
    return false;
  if (loc.Longitude > bb_ur.Longitude)
    return false;
  if (loc.Latitude < bb_ll.Latitude)
    return false;
  if (loc.Latitude > bb_ur.Latitude)
    return false;

  return true;
}
