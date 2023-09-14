// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyholeZone.hpp"
#include "Boundary.hpp"
#include "Geo/GeoVector.hpp"

OZBoundary
KeyholeZone::GetBoundary() const noexcept
{
  OZBoundary boundary;
  boundary.push_front(GetSectorStart());
  boundary.push_front(GetSectorEnd());

  boundary.GenerateArcExcluding(GetReference(), GetRadius(),
                                GetStartRadial(), GetEndRadial());

  const auto small_radius = GetInnerRadius();
  GeoVector small_vector(small_radius, GetStartRadial());
  boundary.push_front(small_vector.EndPoint(GetReference()));
  small_vector.bearing = GetEndRadial();
  boundary.push_front(small_vector.EndPoint(GetReference()));

  boundary.GenerateArcExcluding(GetReference(), small_radius,
                                GetEndRadial(), GetStartRadial());

  return boundary;
}

double
KeyholeZone::ScoreAdjustment() const noexcept
{
  return GetInnerRadius();
}

bool
KeyholeZone::IsInSector(const GeoPoint &location) const noexcept
{
  GeoVector f(GetReference(), location);

  return f.distance <= GetInnerRadius() ||
    (f.distance <= GetRadius() && IsAngleInSector(f.bearing));
}
