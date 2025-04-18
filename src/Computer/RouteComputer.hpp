// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Task/ProtectedRoutePlanner.hpp"
#include "Engine/Task/TaskType.hpp"
#include "Engine/Route/RoutePlanner.hpp"
#include "time/GPSClock.hpp"

struct MoreData;
struct DerivedInfo;
struct GlideSettings;
struct RoutePlannerConfig;
class ProtectedAirspaceWarningManager;
class RasterTerrain;
class GlidePolar;

class RouteComputer {
  static constexpr std::chrono::steady_clock::duration PERIOD = std::chrono::seconds(5);

  RoutePlannerGlue route_planner;
  ProtectedRoutePlanner protected_route_planner;

  GPSClock route_clock;
  GPSClock reach_clock;

  const RasterTerrain *terrain;

  TaskType last_task_type;
  unsigned last_active_tp;

public:
  RouteComputer(const Airspaces &airspace_database,
                const ProtectedAirspaceWarningManager *warnings);

  const ProtectedRoutePlanner &GetProtectedRoutePlanner() const {
    return protected_route_planner;
  }

  /**
   * Release all references to airspace objects from the "master"
   * container.  Call this before modifying the container.
   */
  void ClearAirspaces() {
    route_planner.Reset();
  }

  void ResetFlight();
  void ProcessRoute(const MoreData &basic, DerivedInfo &calculated,
                    const GlideSettings &settings,
                    const RoutePlannerConfig &config,
                    const GlidePolar &glide_polar,
                    const GlidePolar &safety_polar);

  void set_terrain(const RasterTerrain* _terrain);

private:
  void TerrainWarning(const MoreData &basic,
                      DerivedInfo &calculated,
                      const RoutePlannerConfig &config);

  void Reach(const MoreData &basic, DerivedInfo &calculated,
             const RoutePlannerConfig &config);
};
