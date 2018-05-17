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

#include "KeyholeZone.hpp"
#include "Boundary.hpp"
#include "Geo/GeoVector.hpp"

OZBoundary
KeyholeZone::GetBoundary() const
{
  OZBoundary boundary;
  boundary.push_front(GetSectorStart());
  boundary.push_front(GetSectorEnd());

  boundary.GenerateArcExcluding(GetReference(), GetRadius(),
                                GetStartRadial(), GetEndRadial());

  const auto small_radius = GetInnerRadius();
  GeoVector small_vector(small_radius, GetStartRadial());
  boundary.push_front(small_vector.EndPoint(GetReference()));
  small_vector.bearing = GetEndRadial();
  boundary.push_front(small_vector.EndPoint(GetReference()));

  boundary.GenerateArcExcluding(GetReference(), small_radius,
                                GetEndRadial(), GetStartRadial());

  return boundary;
}

double
KeyholeZone::ScoreAdjustment() const
{
  return GetInnerRadius();
}

bool 
KeyholeZone::IsInSector(const GeoPoint &location) const
{
  GeoVector f(GetReference(), location);

  return f.distance <= GetInnerRadius() ||
    (f.distance <= GetRadius() && IsAngleInSector(f.bearing));
}
