/*
  Copyright_License {

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

#include "Boundary.hpp"
#include "Geo/GeoVector.hpp"

void
OZBoundary::GenerateArcExcluding(const GeoPoint &center, double radius,
                                 Angle start_radial, Angle end_radial)
{
  const unsigned steps = 20;
  const Angle delta = Angle::FullCircle() / steps;
  const Angle start = start_radial.AsBearing();
  Angle end = end_radial.AsBearing();
  if (end <= start + Angle::FullCircle() / 512)
    end += Angle::FullCircle();

  GeoVector vector(radius, start + delta);
  for (; vector.bearing < end; vector.bearing += delta)
    push_front(vector.EndPoint(center));
}
