// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Renderer/WaypointReachability.hpp"

struct Waypoint;

/**
 * Reachability for drawing a waypoint icon in a dialog, using the
 * same calculation as the map (current aircraft state and route
 * planner when available).
 */
[[nodiscard]]
WaypointReachability
GetWaypointReachability(const Waypoint &waypoint) noexcept;
