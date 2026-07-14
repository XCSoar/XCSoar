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
  Angle _biSector;
  if (!next && previous)
    // final
    _biSector = GetReference().Bearing(*previous).Reciprocal();
  else if (next && previous)
    // intermediate
    _biSector = GetReference().Bearing(*previous)
      .HalfAngle(GetReference().Bearing(*next));
  else if (next && !previous)
    // start
    _biSector = GetReference().Bearing(*next).Reciprocal();
  else
    // single point
    _biSector = Angle::Zero();

  /*
  Populate the member variable for the bisector angle, so we
  can later update the radials if the sector angle changes.
  */
  biSector = _biSector;

  UpdateRadialsFromSectorAngle();
}

void
SymmetricSectorZone::SetSectorAngle(Angle _angle) noexcept
{
  sector_angle = _angle;
  UpdateRadialsFromSectorAngle();
  UpdateSector();
}

// Updates the radial angles based on the current sector angle and the bisector
void
SymmetricSectorZone::UpdateRadialsFromSectorAngle() noexcept
{
  if (!biSector) {
    // Should never happen
    assert(false);
    return;
  }

  const Angle half = sector_angle.Half();
  SetStartRadial((biSector.value() - half).AsBearing());
  SetEndRadial((biSector.value() + half).AsBearing());
}

bool
SymmetricSectorZone::Equals(const ObservationZonePoint &other) const noexcept
{
  const SymmetricSectorZone &z = (const SymmetricSectorZone &)other;

  return CylinderZone::Equals(other) && sector_angle == z.GetSectorAngle();
}
