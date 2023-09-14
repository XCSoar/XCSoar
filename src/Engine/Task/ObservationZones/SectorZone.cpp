// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SectorZone.hpp"
#include "Boundary.hpp"
#include "Geo/GeoVector.hpp"

OZBoundary
SectorZone::GetBoundary() const noexcept
{
  OZBoundary boundary;

  if (arc_boundary)
    boundary.GenerateArcExcluding(GetReference(), GetRadius(),
                                  GetStartRadial(), GetEndRadial());

  boundary.push_front(GetSectorEnd());
  boundary.push_front(GetSectorStart());
  boundary.push_front(GetReference());

  return boundary;
}

double
SectorZone::ScoreAdjustment() const noexcept
{
  return 0;
}

void
SectorZone::UpdateSector() noexcept
{
  sector_start = GeoVector(GetRadius(), start_radial).EndPoint(GetReference());
  sector_end = GeoVector(GetRadius(), end_radial).EndPoint(GetReference());
}

bool
SectorZone::IsInSector(const GeoPoint &location) const noexcept
{
  GeoVector f(GetReference(), location);

  return f.distance <= GetRadius() && IsAngleInSector(f.bearing);
}

void
SectorZone::SetStartRadial(const Angle x) noexcept
{
  start_radial = x;
  UpdateSector();
}

void
SectorZone::SetEndRadial(const Angle x) noexcept
{
  end_radial = x;
  UpdateSector();
}

bool
SectorZone::IsAngleInSector(const Angle b) const noexcept
{
  // Quit early if we have a full circle
  if ((end_radial - start_radial).AsBearing() <= Angle::FullCircle() / 512)
    return true;

  return b.Between(start_radial, end_radial);
}

bool
SectorZone::Equals(const ObservationZonePoint &other) const noexcept
{
  const SectorZone &z = (const SectorZone &)other;

  return CylinderZone::Equals(other) &&
    start_radial == z.GetStartRadial() &&
    end_radial == z.GetEndRadial();
}
