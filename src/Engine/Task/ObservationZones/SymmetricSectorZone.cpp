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

#include "SymmetricSectorZone.hpp"
#include "Geo/GeoPoint.hpp"

void
SymmetricSectorZone::SetLegs(const GeoPoint *previous, const GeoPoint *next)
{
  /* Important: all bearings must be calculated from "current" as the
     primary reference, because this is the turn point we're currently
     calculating, therefore we need the extra call to Reciprocal().
     Reversing the formula would not work, because the bearing on a
     "Great Circle" is not constant, and we would get a different
     result. */
  Angle biSector;
  if (!next && previous)
    // final
    biSector = GetReference().Bearing(*previous).Reciprocal();
  else if (next && previous)
    // intermediate
    biSector = GetReference().Bearing(*previous)
      .HalfAngle(GetReference().Bearing(*next));
  else if (next && !previous)
    // start
    biSector = GetReference().Bearing(*next).Reciprocal();
  else
    // single point
    biSector = Angle::Zero();

  const Angle half = sector_angle.Half();
  SetStartRadial((biSector - half).AsBearing());
  SetEndRadial((biSector + half).AsBearing());
}

bool
SymmetricSectorZone::Equals(const ObservationZonePoint &other) const
{
  const SymmetricSectorZone &z = (const SymmetricSectorZone &)other;

  return CylinderZone::Equals(other) && sector_angle == z.GetSectorAngle();
}
