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

#include "AnnularSectorZone.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

GeoPoint
AnnularSectorZone::GetBoundaryParametric(fixed t) const
{
  const Angle sweep = (EndRadial-StartRadial).as_bearing();
  const fixed c0 = sweep.value_radians()*InnerRadius;
  const fixed l = Radius-InnerRadius;
  const fixed c1 = sweep.value_radians()*Radius;
  const fixed tt = t*(c0+c1+2*l);
  Angle a;
  fixed d;
  if (tt< c0) {
    d = InnerRadius;
    a = Angle::radians((tt/c0)*sweep.value_radians())+StartRadial;
  } else if (positive(l) && (tt<c0+l)) {
    d = (tt-c0)/l*(Radius-InnerRadius)+InnerRadius;
    a = EndRadial;
  } else if (tt<c0+l+c1) {
    d = Radius;
    a = EndRadial-Angle::radians(((tt-c0-l)/c1)*sweep.value_radians());
  } else if (positive(l)) {
    d = (tt-c0-l-c1)/l*(InnerRadius-Radius)+Radius;
    a = StartRadial;
  } else {
    d = InnerRadius;
    a = StartRadial;
  }
  return GeoVector(d, a).end_point(get_location());
}

bool
AnnularSectorZone::IsInSector(const AircraftState &ref) const
{
  GeoVector f(get_location(), ref.location);

  return (f.Distance <= Radius) && (f.Distance >= InnerRadius) && angleInSector(f.Bearing);
}

bool
AnnularSectorZone::equals(const ObservationZonePoint* other) const
{
  const AnnularSectorZone *z = (const AnnularSectorZone *)other;

  return SectorZone::equals(other) &&
    InnerRadius == z->InnerRadius;
}
