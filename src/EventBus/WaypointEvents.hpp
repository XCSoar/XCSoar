// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

namespace Event {

/**
 * Posted when waypoints have been loaded from files.
 */
struct WaypointsLoadedEvent {
  size_t count;
};

/**
 * Posted when a waypoint needs its elevation updated from terrain.
 */
struct WaypointElevationNeededEvent {
  unsigned waypoint_id;
  GeoPoint location;
};

/**
 * Posted when a waypoint's elevation has been updated.
 */
struct WaypointElevationUpdatedEvent {
  unsigned waypoint_id;
  double elevation;
};

/**
 * Posted when all waypoint elevations in a region have been updated.
 */
struct WaypointElevationsUpdatedEvent {
  size_t count;
};

} // namespace Event
