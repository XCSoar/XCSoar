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

#include "CompareProjection.hpp"
#include "WindowProjection.hpp"

CompareProjection::FourCorners::FourCorners(const WindowProjection &projection)
  :GeoQuadrilateral{projection.ScreenToGeo(0, 0),
    projection.ScreenToGeo(projection.GetScreenWidth(), 0),
    projection.ScreenToGeo(0, projection.GetScreenHeight()),
    projection.ScreenToGeo(projection.GetScreenWidth(),
                           projection.GetScreenHeight())} {}

gcc_pure
static double
SimpleDistance(const GeoPoint &a, const GeoPoint &b,
               const double latitude_cos)
{
  return hypot((a.longitude - b.longitude).AsDelta().Native(),
               (a.latitude - b.latitude).AsDelta().Native() * latitude_cos);
}

CompareProjection::CompareProjection(const WindowProjection &projection)
  :corners(projection),
   latitude_cos(corners.top_left.latitude.fastcosine()),
   max_delta(SimpleDistance(corners.top_left, corners.top_right,
                            latitude_cos) /
             projection.GetScreenWidth())
{
}

bool
CompareProjection::Compare(const CompareProjection &other) const
{
  return max_delta > 0 &&
    SimpleDistance(corners.top_left, other.corners.top_left,
                   latitude_cos) <= max_delta &&
    SimpleDistance(corners.top_right, other.corners.top_right,
                   latitude_cos) <= max_delta &&
    SimpleDistance(corners.bottom_left, other.corners.bottom_left,
                   latitude_cos) <= max_delta &&
    SimpleDistance(corners.bottom_right, other.corners.bottom_right,
                   latitude_cos) <= max_delta;
}

bool
CompareProjection::CompareAndUpdate(const CompareProjection &other)
{
  if (Compare(other))
    return true;

  *this = other;
  return false;
}

