// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Route/ReachResult.hpp"

#include <cstdint>

struct Waypoint;
struct MoreData;
struct DerivedInfo;
struct PolarSettings;
struct TaskBehaviour;
struct SpeedVector;
class MacCready;
class ProtectedRoutePlanner;

enum class WaypointReachability : uint8_t {
  INVALID,
  UNREACHABLE,
  STRAIGHT,
  TERRAIN,
};

static constexpr bool
IsReachable(WaypointReachability r) noexcept
{
  switch (r) {
  case WaypointReachability::INVALID:
  case WaypointReachability::UNREACHABLE:
    break;

  case WaypointReachability::STRAIGHT:
  case WaypointReachability::TERRAIN:
    return true;
  }

  return false;
}

/**
 * The reachability of a waypoint, as used for drawing its icon.
 */
struct WaypointReach {
  ReachResult result{
    .direct = 0,
    .terrain = 0,
    .terrain_valid = ReachResult::Validity::INVALID,
  };

  WaypointReachability reachability = WaypointReachability::INVALID;

  constexpr bool IsReachable() const noexcept {
    return ::IsReachable(reachability);
  }
};

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
 *
 * This is for code which draws a waypoint icon outside of the map
 * (e.g. dialogs), and which therefore has no pre-calculated reach at
 * hand.
 */
WaypointReach
CalculateWaypointReach(const Waypoint &waypoint,
                       const ProtectedRoutePlanner *route_planner,
                       const MoreData &basic, const DerivedInfo &calculated,
                       const PolarSettings &polar_settings,
                       const TaskBehaviour &task_behaviour) noexcept;
