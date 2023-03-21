// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RouteComputer.hpp"
#include "Task/ProtectedRoutePlanner.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/Aircraft.hpp"
#include "Navigation/Aircraft.hpp"

#include <algorithm>

RouteComputer::RouteComputer(const Airspaces &airspace_database,
                             const ProtectedAirspaceWarningManager *warnings)
  :protected_route_planner(route_planner, airspace_database, warnings),
   terrain(NULL)
{}

void
RouteComputer::ResetFlight()
{
  route_clock.Reset();
  reach_clock.Reset();
  protected_route_planner.Reset();

  last_task_type = TaskType::NONE;
  last_active_tp = 0;
}

void
RouteComputer::ProcessRoute(const MoreData &basic, DerivedInfo &calculated,
                            const GlideSettings &settings,
                            const RoutePlannerConfig &config,
                            const GlidePolar &glide_polar,
                            const GlidePolar &safety_polar)
{
  if (!basic.location_available || !basic.NavAltitudeAvailable())
    return;

  protected_route_planner.SetPolars(settings, config,
                                    glide_polar, safety_polar,
                                    calculated.GetWindOrZero(),
                                    calculated.common_stats.height_min_working);

  Reach(basic, calculated, config);
  TerrainWarning(basic, calculated, config);
}

inline void
RouteComputer::TerrainWarning(const MoreData &basic,
                              DerivedInfo &calculated,
                              const RoutePlannerConfig &config)
{
  const AircraftState as = ToAircraftState(basic, calculated);

  const GlideResult& sol = calculated.task_stats.current_leg.solution_remaining;
  if (!sol.IsDefined()) {
    calculated.terrain_warning_location.SetInvalid();
    return;
  }

  const AGeoPoint start (as.location, as.altitude);
  const RoughAltitude h_ceiling(std::max((int)basic.nav_altitude+500,
                                         (int)calculated.common_stats.height_max_working));
  // allow at least 500m of climb above current altitude as ceiling, in case
  // there are no actual working band stats.
  GeoVector v = sol.vector;
  if (v.distance > 200000)
    /* limit to reasonable distances (200km max.) to avoid overflow in
       GeoVector::EndPoint() */
    v.distance = 200000;

  if (terrain) {
    if (sol.IsDefined()) {
      const AGeoPoint dest(v.EndPoint(start), sol.min_arrival_altitude);
      bool dirty = route_clock.CheckAdvance(basic.time, PERIOD);

      if (!dirty) {
        dirty =
          calculated.task_stats.active_index != last_active_tp ||
          calculated.common_stats.task_type != last_task_type;
        if (dirty) {
          // restart clock
          route_clock.Reset();
        }
      }

      last_task_type = calculated.common_stats.task_type;
      last_active_tp = calculated.task_stats.active_index;

      if (dirty) {
        protected_route_planner.SolveRoute(dest, start, config, h_ceiling);
        calculated.planned_route = route_planner.GetSolution();

        calculated.terrain_warning_location =
          route_planner.Intersection(start, dest);
      }
      return;
    } else {
      protected_route_planner.SolveRoute(start, start, config, h_ceiling);
      calculated.planned_route = route_planner.GetSolution();
    }
  }
  calculated.terrain_warning_location.SetInvalid();
}

inline void
RouteComputer::Reach(const MoreData &basic, DerivedInfo &calculated,
                     const RoutePlannerConfig &config)
{
  if (!calculated.terrain_valid) {
    /* without valid terrain information, we cannot calculate
       reachabilty, so let's skip that step completely */
    calculated.terrain_base_valid = false;
    protected_route_planner.ClearReach();
    return;
  }

  const bool do_solve = config.IsReachEnabled() && terrain != NULL;

  const AircraftState state = ToAircraftState(basic, calculated);
  const AGeoPoint start (state.location, state.altitude);
  const int h_ceiling(std::max((int)basic.nav_altitude + 500,
                               (int)calculated.common_stats.height_max_working));

  if (reach_clock.CheckAdvance(basic.time, PERIOD)) {
    protected_route_planner.SolveReach(start, config, h_ceiling, do_solve);

    if (do_solve) {
      calculated.terrain_base = protected_route_planner.GetTerrainBase();
      calculated.terrain_base_valid = true;
    }
  }
}

void
RouteComputer::set_terrain(const RasterTerrain* _terrain) {
  terrain = _terrain;
  protected_route_planner.SetTerrain(terrain);
}
