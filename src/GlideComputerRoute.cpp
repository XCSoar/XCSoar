/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "GlideComputerRoute.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/Aircraft.hpp"
#include "SettingsComputer.hpp"

#include <algorithm>

GlideComputerRoute::GlideComputerRoute(ProtectedTaskManager &task):
  m_task(task),
  route_clock(fixed(5)),
  reach_clock(fixed(5)),
  terrain(NULL)
{}

void
GlideComputerRoute::ResetFlight()
{
  route_clock.reset();
  reach_clock.reset();
}

void
GlideComputerRoute::ProcessRoute(const MoreData &basic,
                                 DerivedInfo &calculated,
                                 const DerivedInfo &last_calculated,
                                 const SETTINGS_COMPUTER &settings_computer)
{
  m_task.route_update_polar(calculated.wind);
  calculated.glide_polar_reach = m_task.get_reach_polar();

  Reach(basic, calculated, settings_computer);
  TerrainWarning(basic, calculated, last_calculated);
}

void
GlideComputerRoute::TerrainWarning(const MoreData &basic,
                                   DerivedInfo &calculated,
                                   const DerivedInfo &last_calculated)
{
  const AircraftState as = ToAircraftState(basic, calculated);

  const GlideResult& sol = calculated.task_stats.current_leg.solution_remaining;
  const AGeoPoint start (as.location, as.altitude);
  const short h_ceiling = (short)std::max((int)basic.NavAltitude+500,
                                          (int)calculated.thermal_band.working_band_ceiling);
  // allow at least 500m of climb above current altitude as ceiling, in case
  // there are no actual working band stats.
  const GeoVector &v = sol.vector;

  if (terrain) {
    if (sol.IsDefined()) {
      const AGeoPoint dest(v.end_point(start), sol.min_height);
      bool dirty = route_clock.check_advance(basic.time);

      if (!dirty) {
        dirty = calculated.common_stats.active_taskpoint_index != last_calculated.common_stats.active_taskpoint_index;
        dirty |= calculated.common_stats.mode_abort != last_calculated.common_stats.mode_abort;
        dirty |= calculated.common_stats.mode_goto != last_calculated.common_stats.mode_goto;
        dirty |= calculated.common_stats.mode_ordered != last_calculated.common_stats.mode_ordered;
        if (dirty) {
          // restart clock
          route_clock.check_advance(basic.time);
          route_clock.reset();
        }
      }

      if (dirty) {
        m_task.route_solve(dest, start, h_ceiling);
        calculated.terrain_warning =
          m_task.intersection(start, dest,
                              calculated.terrain_warning_location);
      }
      return;
    } else {
      m_task.route_solve(start, start, h_ceiling);
    }
  }
  calculated.terrain_warning = false;
}

void
GlideComputerRoute::Reach(const MoreData &basic, DerivedInfo &calculated,
                          const SETTINGS_COMPUTER &settings_computer)
{
  if (!calculated.terrain_valid) {
    /* without valid terrain information, we cannot calculate
       reachabilty, so let's skip that step completely */
    calculated.terrain_base_valid = false;
    return;
  }

  const bool do_solve = (settings_computer.route_planner.reach_enabled() &&
                         terrain != NULL);

  const AircraftState state = ToAircraftState(basic, calculated);
  const AGeoPoint start (state.location, state.altitude);
  if (reach_clock.check_advance(basic.time)) {
    m_task.solve_reach(start, do_solve);

    if (do_solve) {
      calculated.terrain_base = fixed(m_task.get_terrain_base());
      calculated.terrain_base_valid = true;
    }
  }
}

void
GlideComputerRoute::set_terrain(const RasterTerrain* _terrain) {
  terrain = _terrain;
  m_task.route_set_terrain(terrain);
}
