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

#include "KeyholeZone.hpp"
#include "Boundary.hpp"
#include "Geo/GeoVector.hpp"

GeoPoint
KeyholeZone::GetBoundaryParametric(fixed t) const
{
  const fixed sweep = (GetEndRadial() - GetStartRadial()).AsBearing().Radians();
  const fixed small_sweep = fixed_two_pi - sweep;
  const fixed SmallRadius = fixed(500);
  // length of sector element
  const fixed c1 = sweep * GetRadius();
  // length of cylinder element
  const fixed c2 = small_sweep * SmallRadius * 5;
  // length of straight elements
  const fixed l = (GetRadius() - SmallRadius) / 5;
  // total distance
  const fixed tt = t * (c1 + l + l + c2);

  Angle a;
  fixed d;
  if (tt < l) {
    // first straight element
    d = (tt / l) * (GetRadius() - SmallRadius) + SmallRadius;
    a = GetStartRadial();
  } else if (tt < l + c1) {
    // sector element
    d = GetRadius();
    a = GetStartRadial() + Angle::Radians((tt - l) / c1 * sweep);
  } else if (tt < l + l + c1) {
    // second straight element
    d = (fixed(1) - (tt - l - c1) / l) * (GetRadius() - SmallRadius) + SmallRadius;
    a = GetEndRadial();
  } else {
    // cylinder element
    d = SmallRadius;
    a = GetEndRadial() + Angle::Radians((tt - l - l - c1) / c2 * small_sweep);
  }
  return GeoVector(d, a).EndPoint(GetReference());
}

OZBoundary
KeyholeZone::GetBoundary() const
{
  OZBoundary boundary;
  boundary.push_front(GetSectorStart());
  boundary.push_front(GetSectorEnd());

  boundary.GenerateArcExcluding(GetReference(), GetRadius(),
                                GetStartRadial(), GetEndRadial());

  const fixed small_radius = fixed(500);
  GeoVector small_vector(small_radius, GetStartRadial());
  boundary.push_front(small_vector.EndPoint(GetReference()));
  small_vector.bearing = GetEndRadial();
  boundary.push_front(small_vector.EndPoint(GetReference()));

  boundary.GenerateArcExcluding(GetReference(), small_radius,
                                GetEndRadial(), GetStartRadial());

  return std::move(boundary);
}

fixed
KeyholeZone::ScoreAdjustment() const
{
  return fixed(500);
}

bool 
KeyholeZone::IsInSector(const GeoPoint &location) const
{
  GeoVector f(GetReference(), location);

  return f.distance <= fixed(500) ||
    (f.distance <= GetRadius() && IsAngleInSector(f.bearing));
}
