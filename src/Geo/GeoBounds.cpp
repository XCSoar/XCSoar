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

#include "GeoBounds.hpp"

bool
GeoBounds::Extend(const GeoPoint pt)
{
  if (!pt.IsValid())
    return false;

  if (IsValid()) {
    bool a = longitude.Extend(pt.longitude);
    bool b = latitude.Extend(pt.latitude);
    return a || b;
  } else {
    *this = GeoBounds(pt);
    return true;
  }
}

bool
GeoBounds::IntersectWith(const GeoBounds &other)
{
  return longitude.IntersectWith(other.longitude) &&
    latitude.IntersectWith(other.latitude);
}

GeoPoint
GeoBounds::GetCenter() const
{
  if (!IsValid())
    return GeoPoint::Invalid();

  return GeoPoint(longitude.GetMiddle().AsDelta(), latitude.GetMiddle());
}

GeoBounds
GeoBounds::Scale(double factor) const
{
  if (!IsValid())
    return Invalid();

  Angle diff_lat_half = GetHeight() / 2 * (factor - 1);
  Angle diff_lon_half = GetWidth() / 2 * (factor - 1);

  GeoBounds br = *this;
  br.longitude.end += diff_lon_half;
  br.longitude.start -= diff_lon_half;
  br.latitude.end += diff_lat_half;
  br.latitude.start -= diff_lat_half;

  return br;
}
