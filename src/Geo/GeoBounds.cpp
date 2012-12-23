/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

static bool
IntersectWith(Angle &a, Angle &b, const Angle other_a, const Angle other_b)
{
  bool result = false;

  if (other_a.Between(a, b)) {
    a = other_a;
    result = true;
  }

  if (other_b.Between(a, b)) {
    b = other_b;
    result = true;
  }

  return result;
}

bool
GeoBounds::IntersectWith(const GeoBounds &other)
{
  return ::IntersectWith(west, east, other.west, other.east) &&
    ::IntersectWith(south, north, other.south, other.north);
}

GeoPoint
GeoBounds::GetCenter() const
{
  if (!IsValid())
    return GeoPoint::Invalid();

  return GeoPoint(west.Fraction(east, fixed(0.5)),
                  south.Fraction(north, fixed(0.5)));
}

GeoBounds
GeoBounds::Scale(fixed factor) const
{
  if (!IsValid())
    return Invalid();

  Angle diff_lat_half =
    (north - south).AsBearing() / fixed(2) * (factor - fixed(1));
  Angle diff_lon_half =
    (east - west).AsBearing() / fixed(2) * (factor - fixed(1));

  GeoBounds br = *this;
  br.east += diff_lon_half;
  br.west -= diff_lon_half;
  br.north += diff_lat_half;
  br.south -= diff_lat_half;

  return br;
}
