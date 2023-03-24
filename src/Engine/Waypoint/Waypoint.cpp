// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Waypoint.hpp"
#include "Geo/Flat/FlatProjection.hpp"

Waypoint::Waypoint(const GeoPoint &_location) noexcept
  :location(_location)
{
}

bool
Waypoint::IsCloseTo(const GeoPoint &_location,
                    const double range) const noexcept
{
  return location.Distance(_location) <= range;
}

void
Waypoint::Project(const FlatProjection &projection) noexcept
{
  flat_location = projection.ProjectInteger(location);

#ifndef NDEBUG
  flat_location_initialised = true;
#endif
}
