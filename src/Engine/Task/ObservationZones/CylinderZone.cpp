// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CylinderZone.hpp"
#include "Boundary.hpp"
#include "Geo/GeoVector.hpp"

#include <algorithm>

#include <stdlib.h>

double
CylinderZone::ScoreAdjustment() const noexcept
{
  return radius;
}

GeoPoint
CylinderZone::GetNearestPoint(const FlatProjection &,
                              const GeoPoint &location) const noexcept
{
  const GeoVector v{GetReference(), location};

  /* inside the cylinder the nearest point of the rim is behind the
     aircraft: for a start that is the wrong way out, away from the
     next task point.  Leave those to the caller. */
  if (v.distance <= radius)
    return GeoPoint::Invalid();

  return GeoVector{radius, v.bearing}.EndPoint(GetReference());
}

OZBoundary
CylinderZone::GetBoundary() const noexcept
{
  OZBoundary boundary;

  const unsigned steps = 20;
  const auto delta = Angle::FullCircle() / steps;

  GeoVector vector(GetRadius(), Angle::Zero());
  for (unsigned i = 0; i < steps; ++i, vector.bearing += delta)
    boundary.push_front(vector.EndPoint(GetReference()));

  return boundary;
}

bool
CylinderZone::Equals(const ObservationZonePoint &other) const noexcept
{
  const CylinderZone &z = (const CylinderZone &)other;

  return ObservationZonePoint::Equals(other) && GetRadius() == z.GetRadius();
}

GeoPoint
CylinderZone::GetRandomPointInSector(const double mag) const noexcept
{
  GeoPoint location;

  do {
    auto dir = Angle::Degrees(rand() % 360);
    double dmag = std::max(std::min(radius, 100.0), radius * mag);
    double dis = (0.1 + (rand() % 90) / 100.0) * dmag;
    GeoVector vec(dis, dir);
    location = vec.EndPoint(GetReference());
  } while (!IsInSector(location));

  return location;
}
