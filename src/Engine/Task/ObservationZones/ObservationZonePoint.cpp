// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ObservationZonePoint.hpp"

GeoPoint
ObservationZonePoint::GetNearestPoint(const FlatProjection &,
                                      const GeoPoint &) const noexcept
{
  return GeoPoint::Invalid();
}

bool
ObservationZonePoint::Equals(const ObservationZonePoint &other) const noexcept
{
  return GetShape() == other.GetShape() &&
    GetReference() == other.GetReference();
}
