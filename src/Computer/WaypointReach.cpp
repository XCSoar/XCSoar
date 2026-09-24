// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReach.hpp"
#include "Settings.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Engine/Task/TaskBehaviour.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Geo/GeoPoint.hpp"

#include <cassert>

WaypointReach
CalculateWaypointReachRoute(const Waypoint &waypoint,
                            const ProtectedRoutePlanner &route_planner,
                            const TaskBehaviour &task_behaviour) noexcept
{
  WaypointReach reach;

  if (!waypoint.has_elevation)
    return reach;

  const double elevation = waypoint.elevation +
    task_behaviour.safety_height_arrival;
  const AGeoPoint p_dest(waypoint.location, elevation);

  const auto result = route_planner.FindPositiveArrival(p_dest);
  if (!result)
    return reach;

  reach.result = *result;
  reach.result.Subtract(elevation);

  if (!reach.result.IsReachableDirect())
    reach.reachability = WaypointReachability::UNREACHABLE;
  else if (task_behaviour.route_planner.IsReachEnabled() &&
           !reach.result.IsReachableTerrain())
    reach.reachability = WaypointReachability::STRAIGHT;
  else
    reach.reachability = WaypointReachability::TERRAIN;

  return reach;
}

WaypointReach
CalculateWaypointReachDirect(const Waypoint &waypoint, const MoreData &basic,
                             const SpeedVector &wind,
                             const MacCready &mac_cready,
                             const TaskBehaviour &task_behaviour) noexcept
{
  assert(basic.location_available);
  assert(basic.NavAltitudeAvailable());

  WaypointReach reach;

  if (!waypoint.has_elevation)
    return reach;

  const auto elevation = waypoint.elevation +
    task_behaviour.safety_height_arrival;
  const GlideState state(GeoVector(basic.location, waypoint.location),
                         elevation, basic.nav_altitude, wind);

  const GlideResult result = mac_cready.SolveStraight(state);
  if (!result.IsOk())
    return reach;

  reach.result.direct = result.pure_glide_altitude_difference;
  reach.reachability = result.pure_glide_altitude_difference > 0
    ? WaypointReachability::TERRAIN
    : WaypointReachability::UNREACHABLE;

  return reach;
}

WaypointReach
CalculateWaypointReach(const Waypoint &waypoint,
                       const ProtectedRoutePlanner *route_planner,
                       const MoreData &basic, const DerivedInfo &calculated,
                       const PolarSettings &polar_settings,
                       const TaskBehaviour &task_behaviour) noexcept
{
  if (route_planner != nullptr && !route_planner->IsTerrainReachEmpty())
    return CalculateWaypointReachRoute(waypoint, *route_planner,
                                       task_behaviour);

  if (!basic.location_available || !basic.NavAltitudeAvailable())
    return {};

  const GlidePolar &glide_polar =
    task_behaviour.route_planner.reach_polar_mode == RoutePlannerConfig::Polar::TASK
    ? polar_settings.glide_polar_task
    : calculated.glide_polar_safety;

  return CalculateWaypointReachDirect(waypoint, basic,
                                      calculated.GetWindOrZero(),
                                      MacCready(task_behaviour.glide,
                                                glide_polar),
                                      task_behaviour);
}
