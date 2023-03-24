// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RoutePlannerGlue.hpp"
#include "Engine/Route/ReachFan.hpp"
#include "Engine/Route/RoutePolars.hpp"
#include "thread/Mutex.hxx"

struct GlideSettings;
struct RoutePlannerConfig;
class GlidePolar;
class RasterTerrain;
class Airspaces;

/**
 * Facade to task/airspace/waypoints as used by threads,
 * to manage locking
 */
class ProtectedRoutePlanner
{
  const Airspaces &airspaces;
  const ProtectedAirspaceWarningManager *warnings;

  mutable Mutex route_mutex;
  RoutePlannerGlue &route_planner;

  /**
   * This mutex protects the "reach" fields.  It is a separate mutex
   * to reduce lock contention between #CalculationThread and
   * #DrawThread.
   */
  mutable Mutex reach_mutex;

  RoutePolars rpolars_reach;
  ReachFan reach_terrain;
  ReachFan reach_working;

public:
  ProtectedRoutePlanner(RoutePlannerGlue &route, const Airspaces &_airspaces,
                        const ProtectedAirspaceWarningManager *_warnings) noexcept
    :airspaces(_airspaces), warnings(_warnings),
     route_planner(route) {}

  void Reset() noexcept {
    ClearReach();

    const std::scoped_lock lock{route_mutex};
    route_planner.Reset();
  }

  void ClearReach() noexcept {
    const std::scoped_lock lock{reach_mutex};
    reach_terrain.Reset();
    reach_working.Reset();
  }

  [[gnu::pure]]
  bool IsTerrainReachEmpty() const noexcept {
    const std::scoped_lock lock{reach_mutex};
    return reach_terrain.IsEmpty();
  }

  void SetTerrain(const RasterTerrain *terrain) noexcept;

  void SetPolars(const GlideSettings &settings,
                 const RoutePlannerConfig &config,
                 const GlidePolar &glide_polar, const GlidePolar &safety_polar,
                 const SpeedVector &wind,
                 int height_min_working) noexcept;

  void SolveRoute(const AGeoPoint &dest, const AGeoPoint &start,
                  const RoutePlannerConfig &config,
                  int h_ceiling) noexcept;

  void SolveReach(const AGeoPoint &origin, const RoutePlannerConfig &config,
                  int h_ceiling, bool do_solve) noexcept;

  [[gnu::pure]]
  const FlatProjection GetTerrainReachProjection() const noexcept;

  [[gnu::pure]]
  std::optional<ReachResult> FindPositiveArrival(const AGeoPoint &dest) const noexcept;

  void AcceptInRange(const GeoBounds &bounds,
                     FlatTriangleFanVisitor &visitor,
                     bool working) const noexcept;

  [[gnu::pure]]
  int GetTerrainBase() const noexcept;
};
