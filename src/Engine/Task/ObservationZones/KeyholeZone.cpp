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

#include "KeyholeZone.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

GeoPoint 
KeyholeZone::get_boundary_parametric(fixed t) const
{ 
  const fixed sweep = (getEndRadial() - getStartRadial()).as_bearing().value_radians();
  const fixed small_sweep = fixed_two_pi-sweep;
  const fixed SmallRadius = fixed(500);
  const fixed c1 = sweep*Radius; // length of sector element
  const fixed c2 = small_sweep*SmallRadius*fixed(5); // length of cylinder element
  const fixed l = (Radius-SmallRadius)*fixed(0.2); // length of straight elements
  const fixed tt = t*(c1+l+l+c2); // total distance
  Angle a;
  fixed d;
  if (tt<l) { // first straight element
    d = (tt/l)*(Radius-SmallRadius)+SmallRadius;
    a = getStartRadial();
  } else if (tt<l+c1) { // sector element
    d = Radius;
    a = getStartRadial() + Angle::radians((tt-l)/c1*sweep);
  } else if (tt<l+l+c1) { // second straight element
    d = (fixed_one-(tt-l-c1)/l)*(Radius-SmallRadius)+SmallRadius;
    a = getEndRadial();
  } else { // cylinder element
    d = SmallRadius;
    a = getEndRadial() + Angle::radians((tt-l-l-c1)/c2*small_sweep);
  }
  return GeoVector(d, a).end_point(get_location());
}

fixed 
KeyholeZone::score_adjustment() const
{
  return fixed(500);
}

bool 
KeyholeZone::isInSector(const AIRCRAFT_STATE &ref) const
{
  GeoVector f(get_location(), ref.Location);

  return f.Distance <= fixed(500) ||
         (f.Distance <= Radius && angleInSector(f.Bearing));
}
