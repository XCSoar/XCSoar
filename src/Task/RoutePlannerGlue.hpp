/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

#ifndef ROUTE_PLANNER_GLUE_HPP
#define ROUTE_PLANNER_GLUE_HPP

#include "Route/AirspaceRoute.hpp"

struct GlideSettings;
class RasterTerrain;
class ProtectedAirspaceWarningManager;

class RoutePlannerGlue {
  const RasterTerrain *terrain;
  AirspaceRoute planner;

public:
  RoutePlannerGlue():terrain(nullptr) {}

  void SetTerrain(const RasterTerrain *terrain);

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

  bool IsTerrainReachEmpty() const {
    return planner.IsTerrainReachEmpty();
  }

  void ClearReach() {
    planner.ClearReach();
  }

  void Reset() {
    planner.Reset();
  }

  bool Solve(const AGeoPoint &origin, const AGeoPoint &destination,
             const RoutePlannerConfig &config,
             int h_ceiling);

  const Route &GetSolution() const {
    return planner.GetSolution();
  }

  void SolveReach(const AGeoPoint &origin, const RoutePlannerConfig &config,
                  int h_ceiling, bool do_solve);

  bool FindPositiveArrival(const AGeoPoint &dest, ReachResult &result_r) const;

  const FlatProjection &GetTerrainReachProjection() const {
    return planner.GetTerrainReachProjection();
  }

  void AcceptInRange(const GeoBounds &bounds,
                     FlatTriangleFanVisitor &visitor,
                     bool working) const {
    planner.AcceptInRange(bounds, visitor, working);
  }

  gcc_pure
  GeoPoint Intersection(const AGeoPoint &origin,
                        const AGeoPoint &destination) const;

  int GetTerrainBase() const;
};

#endif
