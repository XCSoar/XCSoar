// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointDialogs.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/LastUsed.hpp"

/**
 * Opens up the WaypointDetails window of the nearest
 * waypoint to location
 * @param way_points Waypoints including all possible
 * waypoints for the calculation
 * @param location Location where to search
 * @param range Maximum range to search
 * @param pan True if in Pan mode
 * @return True if a waypoint was found
 */
bool
PopupNearestWaypointDetails(Waypoints &waypoints,
                            const GeoPoint &location,
                            double range)
{
  auto way_point = waypoints.LookupLocation(location, range);

  if (way_point) {
    LastUsedWaypoints::Add(*way_point);
    dlgWaypointDetailsShowModal(&waypoints, way_point, true, true);
    return true;
  }
  return false;
}

