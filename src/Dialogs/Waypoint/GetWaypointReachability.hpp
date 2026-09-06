// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Renderer/WaypointReachability.hpp"

struct Waypoint;

/**
 * Reach for drawing a waypoint in a dialog, using the same
 * calculation as the map (current aircraft state and route planner
 * when available).
 */
[[nodiscard]]
WaypointReach
GetWaypointReach(const Waypoint &waypoint) noexcept;

/**
 * Reachability enum for drawing a waypoint icon in a dialog.
 */
[[nodiscard]]
inline WaypointReachability
GetWaypointReachability(const Waypoint &waypoint) noexcept
{
  return GetWaypointReach(waypoint).reachability;
}
