// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointListBuilder.hpp"
#include "WaypointList.hpp"
#include "WaypointFilter.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

void
WaypointListBuilder::Visit(const Waypoints &waypoints) noexcept
{
  if (filter.distance > 0)
    waypoints.VisitWithinRange(location, filter.distance, *this);
  else
    waypoints.VisitNamePrefix(filter.name, *this);
}

inline void
WaypointListBuilder::operator()(const WaypointPtr &waypoint) noexcept
{
  if (filter.Matches(*waypoint, location, triangle_validator))
    list.emplace_back(waypoint);
}
