// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

/**
 * Maximum number of map waypoint symbols (and matching label slots) prepared
 * per frame.  #WaypointVisitorMap and #WaypointLabelList must use the same
 * cap.  Larger values increase DrawThread stack use (~size of #VisibleWaypoint
 * and #WaypointLabelList::Label per entry).
 */
inline constexpr std::size_t MAX_MAP_WAYPOINT_DRAW = 1024;
