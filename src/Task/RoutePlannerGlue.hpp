// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Route/AirspaceRoute.hpp"

struct GlideSettings;
class TerrainBase;
class ProtectedAirspaceWarningManager;

class RoutePlannerGlue {
  AirspaceRoute planner;

public:
  void SetTerrain(const TerrainBase *terrain);

  void UpdatePolar(const GlideSettings &settings,
                   const RoutePlannerConfig &config,
                   const GlidePolar &polar,
                   const GlidePolar &safety_polar,
                   const SpeedVector &wind,
                   const int height_min_working) {
    planner.UpdatePolar(settings, config, polar, safety_polar,
                        wind, height_min_working);
  }

  void Synchronise(const Airspaces &master,
                   const ProtectedAirspaceWarningManager *warnings,
                   const AGeoPoint &origin,
                   const AGeoPoint &destination);

  void Reset() {
    planner.Reset();
  }

  bool Solve(const AGeoPoint &origin, const AGeoPoint &destination,
             const RoutePlannerConfig &config,
             int h_ceiling);

  const Route &GetSolution() const {
    return planner.GetSolution();
  }

  [[gnu::pure]]
  ReachFan SolveReach(const AGeoPoint &origin, const RoutePlannerConfig &config,
                      int h_ceiling, bool do_solve, bool working) noexcept;

  const auto &GetReachPolar() const noexcept {
    return planner.GetReachPolar();
  }

  [[gnu::pure]]
  GeoPoint Intersection(const AGeoPoint &origin,
                        const AGeoPoint &destination) const;
};
