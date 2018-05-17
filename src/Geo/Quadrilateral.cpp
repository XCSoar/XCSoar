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

#include "Quadrilateral.hpp"
#include "GeoBounds.hpp"

#include <algorithm>

GeoBounds
GeoQuadrilateral::GetBounds() const
{
  // TODO: not wraparound-safe

  const auto longitude = std::minmax({top_left.longitude, top_right.longitude, bottom_left.longitude, bottom_right.longitude});
  const auto latitude = std::minmax({top_left.latitude, top_right.latitude, bottom_left.latitude, bottom_right.latitude});

  return GeoBounds(GeoPoint(longitude.first, latitude.second),
                   GeoPoint(longitude.second, latitude.first));
}
