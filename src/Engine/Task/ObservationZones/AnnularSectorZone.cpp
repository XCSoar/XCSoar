// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AnnularSectorZone.hpp"
#include "Boundary.hpp"
#include "Geo/GeoVector.hpp"

OZBoundary
AnnularSectorZone::GetBoundary() const noexcept
{
  OZBoundary boundary;

  const unsigned steps = 20;
  const Angle delta = Angle::FullCircle() / steps;
  const Angle start = GetStartRadial().AsBearing();
  Angle end = GetEndRadial().AsBearing();
  if (end <= start + Angle::FullCircle() / 512)
    end += Angle::FullCircle();

  const GeoPoint inner_start =
    GeoVector(GetInnerRadius(), GetStartRadial()).EndPoint(GetReference());
  const GeoPoint inner_end =
    GeoVector(GetInnerRadius(), GetEndRadial()).EndPoint(GetReference());

  GeoVector inner_vector(GetInnerRadius(), start + delta);
  for (; inner_vector.bearing < end; inner_vector.bearing += delta)
    boundary.push_front(inner_vector.EndPoint(GetReference()));

  boundary.push_front(inner_end);
  boundary.push_front(inner_start);

  GeoVector vector(GetRadius(), start + delta);
  for (; vector.bearing < end; vector.bearing += delta)
    boundary.push_front(vector.EndPoint(GetReference()));

  boundary.push_front(GetSectorEnd());
  boundary.push_front(GetSectorStart());

  return boundary;
}

bool
AnnularSectorZone::IsInSector(const GeoPoint &location) const noexcept
{
  GeoVector f(GetReference(), location);

  return (f.distance <= GetRadius()) &&
    (f.distance >= inner_radius) &&
    IsAngleInSector(f.bearing);
}

bool
AnnularSectorZone::Equals(const ObservationZonePoint &other) const noexcept
{
  const AnnularSectorZone &z = (const AnnularSectorZone &)other;

  return SectorZone::Equals(other) && inner_radius == z.inner_radius;
}
