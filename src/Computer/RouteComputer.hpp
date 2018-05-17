/*
Copyright_License {

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

#ifndef XCSOAR_ROUTE_COMPUTER_HPP
#define XCSOAR_ROUTE_COMPUTER_HPP

#include "Task/ProtectedRoutePlanner.hpp"
#include "Engine/Task/TaskType.hpp"
#include "Engine/Route/RoutePlanner.hpp"
#include "Time/GPSClock.hpp"

struct MoreData;
struct DerivedInfo;
struct GlideSettings;
struct RoutePlannerConfig;
class ProtectedAirspaceWarningManager;
class RasterTerrain;
class GlidePolar;

class RouteComputer {
  static constexpr unsigned PERIOD = 5;

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

  /**
   * Returns a reference to the unprotected route planner object,
   * which must not be used outside of the calculation thread.
   */
  const RoutePlannerGlue &GetRoutePlanner() const {
    return route_planner;
  }

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

#endif
