// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LastUsed.hpp"
#include "Waypoint/Waypoint.hpp"

namespace LastUsedWaypoints
{
  WaypointIDList waypoint_ids;
}

void
LastUsedWaypoints::Add(unsigned waypoint_id)
{
  // Remove it if it exists already
  waypoint_ids.remove(waypoint_id);
  // and add it to the top of the list
  waypoint_ids.push_back(waypoint_id);
}

void
LastUsedWaypoints::Add(const Waypoint &waypoint)
{
  Add(waypoint.id);
}

const WaypointIDList &
LastUsedWaypoints::GetList()
{
  return waypoint_ids;
}

