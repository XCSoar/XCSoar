/*
Copyright_License {

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

#include "GeoBounds.hpp"

void
GeoBounds::Extend(const GeoPoint pt)
{
  if (!pt.IsValid())
    return;

  if (IsValid()) {
    if (pt.longitude < west)
      west = pt.longitude;
    if (pt.latitude > north)
      north = pt.latitude;
    if (pt.longitude > east)
      east = pt.longitude;
    if (pt.latitude < south)
      south = pt.latitude;
  } else {
    west = east = pt.longitude;
    north = south = pt.latitude;
  }
}

GeoPoint
GeoBounds::GetCenter() const
{
  if (!IsValid())
    return GeoPoint::Invalid();

  return GeoPoint(west.Fraction(east, fixed_half),
                  south.Fraction(north, fixed_half));
}

GeoBounds
GeoBounds::Scale(fixed factor) const
{
  if (!IsValid())
    return Invalid();

  Angle diff_lat_half =
    (north - south).AsBearing() / fixed_two * (factor - fixed_one);
  Angle diff_lon_half =
    (east - west).AsBearing() / fixed_two * (factor - fixed_one);

  GeoBounds br = *this;
  br.east += diff_lon_half;
  br.west -= diff_lon_half;
  br.north += diff_lat_half;
  br.south -= diff_lat_half;

  return br;
}
