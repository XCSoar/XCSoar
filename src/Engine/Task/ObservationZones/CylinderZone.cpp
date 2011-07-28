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

#include "CylinderZone.hpp"

#include "Navigation/Geometry/GeoVector.hpp"

#include <stdlib.h>
#include <algorithm>

fixed 
CylinderZone::score_adjustment() const
{
  return Radius;
}

GeoPoint 
CylinderZone::get_boundary_parametric(fixed t) const
{ 
  return GeoVector(Radius,
                   Angle::radians(t * fixed_two_pi)).end_point(get_location());
}

bool
CylinderZone::equals(const ObservationZonePoint* other) const
{
  const CylinderZone* z = (const CylinderZone*)other;

  return ObservationZonePoint::equals(other) &&
         Radius == z->getRadius();
}

GeoPoint
CylinderZone::randomPointInSector(const fixed mag) const
{
  AircraftState ac;
  do {
    Angle dir = Angle::degrees(fixed(rand() % 360));
    fixed dmag = max(min(Radius, fixed(100.0)), Radius * mag);
    fixed dis = fixed((0.1 + (rand() % 90) / 100.0)) * dmag;
    GeoVector vec(dis, dir);
    ac.location = vec.end_point(get_location());
  } while (!isInSector(ac));
  return ac.location;
}
