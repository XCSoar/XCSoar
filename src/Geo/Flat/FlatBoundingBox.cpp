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

#include "FlatBoundingBox.hpp"
#include "FlatRay.hpp"
#include "Math/fixed.hpp"
#include "Math/FastMath.h"

#include <algorithm>

unsigned
FlatBoundingBox::Distance(const FlatBoundingBox &f) const
{
  if (Overlaps(f))
    return 0;

  int dx = std::max(0, std::min(f.bb_ll.longitude - bb_ur.longitude,
                                bb_ll.longitude - f.bb_ur.longitude));
  int dy = std::max(0, std::min(f.bb_ll.latitude - bb_ur.latitude,
                                bb_ll.latitude - f.bb_ur.latitude));

  return ihypot(dx, dy);
}

bool
FlatBoundingBox::Intersects(const FlatRay& ray) const
{
  fixed tmin = fixed(0);
  fixed tmax = fixed(1);
  
  // Longitude
  if (ray.vector.longitude == 0) {
    // ray is parallel to slab. No hit if origin not within slab
    if (ray.point.longitude < bb_ll.longitude ||
        ray.point.longitude > bb_ur.longitude)
      return false;
  } else {
    // compute intersection t value of ray with near/far plane of slab
    fixed t1 = (bb_ll.longitude - ray.point.longitude) * ray.fx;
    fixed t2 = (bb_ur.longitude - ray.point.longitude) * ray.fx;
    // make t1 be intersection with near plane, t2 with far plane
    if (t1 > t2)
      std::swap(t1, t2);

    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    // exit with no collision as soon as slab intersection becomes empty
    if (tmin > tmax)
      return false;
  }

  // Latitude
  // Longitude
  if (ray.vector.latitude == 0) {
    // ray is parallel to slab. No hit if origin not within slab
    if (ray.point.latitude < bb_ll.latitude ||
        ray.point.latitude > bb_ur.latitude)
      return false;
  } else {
    // compute intersection t value of ray with near/far plane of slab
    fixed t1 = (bb_ll.latitude - ray.point.latitude) * ray.fy;
    fixed t2 = (bb_ur.latitude - ray.point.latitude) * ray.fy;
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
  return FlatGeoPoint((bb_ll.longitude + bb_ur.longitude) / 2,
                      (bb_ll.latitude + bb_ur.latitude) / 2);
}

bool
FlatBoundingBox::Overlaps(const FlatBoundingBox& other) const
{
  if (bb_ll.longitude > other.bb_ur.longitude)
    return false;
  if (bb_ur.longitude < other.bb_ll.longitude)
    return false;
  if (bb_ll.latitude > other.bb_ur.latitude)
    return false;
  if (bb_ur.latitude < other.bb_ll.latitude)
    return false;

  return true;
}

bool
FlatBoundingBox::IsInside(const FlatGeoPoint& loc) const
{
  if (loc.longitude < bb_ll.longitude)
    return false;
  if (loc.longitude > bb_ur.longitude)
    return false;
  if (loc.latitude < bb_ll.latitude)
    return false;
  if (loc.latitude > bb_ur.latitude)
    return false;

  return true;
}
