// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Route/WaypointReachability.hpp"

struct Waypoint;
struct MoreData;
struct DerivedInfo;
struct PolarSettings;
struct TaskBehaviour;
struct SpeedVector;
class MacCready;
class ProtectedRoutePlanner;

/**
 * Calculate the reachability of the given waypoint using the route
 * planner, i.e. taking terrain into account.
 */
WaypointReach
CalculateWaypointReachRoute(const Waypoint &waypoint,
                            const ProtectedRoutePlanner &route_planner,
                            const TaskBehaviour &task_behaviour) noexcept;

/**
 * Calculate the reachability of the given waypoint with a straight
 * glide, ignoring terrain.
 */
WaypointReach
CalculateWaypointReachDirect(const Waypoint &waypoint, const MoreData &basic,
                             const SpeedVector &wind,
                             const MacCready &mac_cready,
                             const TaskBehaviour &task_behaviour) noexcept;

/**
 * Calculate the reachability of the given waypoint the same way the
 * map does: via the route planner as long as terrain reach data is
 * available, and with a straight glide otherwise.
 */
WaypointReach
CalculateWaypointReach(const Waypoint &waypoint,
                       const ProtectedRoutePlanner *route_planner,
                       const MoreData &basic, const DerivedInfo &calculated,
                       const PolarSettings &polar_settings,
                       const TaskBehaviour &task_behaviour) noexcept;
