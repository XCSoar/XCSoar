// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SymmetricSectorZone.hpp"
#include "Geo/GeoPoint.hpp"

void
SymmetricSectorZone::SetLegs(const GeoPoint *previous,
                             const GeoPoint *next) noexcept
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
SymmetricSectorZone::Equals(const ObservationZonePoint &other) const noexcept
{
  const SymmetricSectorZone &z = (const SymmetricSectorZone &)other;

  return CylinderZone::Equals(other) && sector_angle == z.GetSectorAngle();
}
