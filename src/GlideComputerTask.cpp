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

#include "GlideComputerTask.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "NMEA/Aircraft.hpp"

#include <algorithm>

using std::max;

// JMW TODO: abstract up to higher layer so a base copy of this won't
// call any event

GlideComputerTask::GlideComputerTask(ProtectedTaskManager &task):
  GlideComputerBlackboard(task),
  route_clock(fixed(5)),
  reach_clock(fixed(5)),
  terrain(NULL)
{}

void
GlideComputerTask::Initialise()
{
}

void
GlideComputerTask::ResetFlight(const bool full)
{
  m_task.reset();
  route_clock.reset();
  reach_clock.reset();
}

void
GlideComputerTask::ProcessBasicTask()
{
  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &derived = SetCalculated();

  ProtectedTaskManager::ExclusiveLease task(m_task);

  task->set_task_behaviour(SettingsComputer());

  if (basic.Time != LastBasic().Time && basic.LocationAvailable) {
    const AIRCRAFT_STATE current_as = ToAircraftState(basic, Calculated());
    const AIRCRAFT_STATE last_as = ToAircraftState(LastBasic(),
                                                   LastCalculated());

    task->update(current_as, last_as);
    if (task->update_auto_mc(current_as,
                             std::max(Calculated().LastThermalAverageSmooth,
                                      fixed_zero))) {
      derived.auto_mac_cready = task->get_glide_polar().get_mc();
      derived.auto_mac_cready_available.update(basic.Time);
    }
  }

  SetCalculated().task_stats = task->get_stats();
  SetCalculated().common_stats = task->get_common_stats();
  SetCalculated().contest_stats = task->get_contest_stats();

  SetCalculated().glide_polar_safety = task->get_safety_polar();
}

void
GlideComputerTask::ProcessMoreTask()
{
  m_task.route_update_polar(Calculated().wind);
  SetCalculated().glide_polar_reach = m_task.get_reach_polar();

  Reach();
  TerrainWarning();

  if (SettingsComputer().EnableBlockSTF)
    SetCalculated().V_stf = Calculated().common_stats.V_block;
  else
    SetCalculated().V_stf = Calculated().common_stats.V_dolphin;

  if (Calculated().task_stats.current_leg.solution_remaining.defined()) {
    const GeoVector &v = Calculated().task_stats.current_leg.solution_remaining.Vector;
    SetCalculated().AutoZoomDistance = v.Distance;
  }
}

void
GlideComputerTask::ProcessIdle()
{
  const AIRCRAFT_STATE as = ToAircraftState(Basic(), Calculated());
  ProtectedTaskManager::ExclusiveLease task(m_task);
  task->update_idle(as);
}

void
GlideComputerTask::TerrainWarning()
{
  const AIRCRAFT_STATE as = ToAircraftState(Basic(), Calculated());

  const GlideResult& sol = Calculated().task_stats.current_leg.solution_remaining;
  const AGeoPoint start (as.get_location(), as.NavAltitude);
  const short h_ceiling = (short)std::max((int)Basic().NavAltitude+500,
                                          (int)Calculated().thermal_band.working_band_ceiling);
  // allow at least 500m of climb above current altitude as ceiling, in case
  // there are no actual working band stats.
  const GeoVector &v = sol.Vector;

  if (terrain) {
    if (sol.defined()) {
      const AGeoPoint dest(v.end_point(start), sol.MinHeight);
      bool dirty = route_clock.check_advance(Basic().Time);

      if (!dirty) {
        dirty = Calculated().common_stats.active_taskpoint_index != LastCalculated().common_stats.active_taskpoint_index;
        dirty |= Calculated().common_stats.mode_abort != LastCalculated().common_stats.mode_abort;
        dirty |= Calculated().common_stats.mode_goto != LastCalculated().common_stats.mode_goto;
        dirty |= Calculated().common_stats.mode_ordered != LastCalculated().common_stats.mode_ordered;
        if (dirty) {
          // restart clock
          route_clock.check_advance(Basic().Time);
          route_clock.reset();
        }
      }

      if (dirty) {
        m_task.route_solve(dest, start, h_ceiling);
        SetCalculated().TerrainWarning = m_task.intersection(start, dest,
                                                             SetCalculated().TerrainWarningLocation);
      }
      return;
    } else {
      m_task.route_solve(start, start, h_ceiling);
    }
  }
  SetCalculated().TerrainWarning = false;
}

void
GlideComputerTask::Reach()
{
  const bool do_solve = (SettingsComputer().route_planner.reach_enabled() &&
                         terrain != NULL);

  const AIRCRAFT_STATE state = ToAircraftState(Basic(), Calculated());
  const AGeoPoint start (state.get_location(), state.NavAltitude);
  if (reach_clock.check_advance(Basic().Time)) {
    m_task.solve_reach(start, do_solve);

    SetCalculated().TerrainBase =
      do_solve? fixed(m_task.get_terrain_base()) : Calculated().TerrainAlt;
  }
}

void 
GlideComputerTask::OnTakeoff()
{
  ProtectedTaskManager::ExclusiveLease task(m_task);
  task->takeoff_autotask(Basic().Location, Calculated().TerrainAlt);
}

void 
GlideComputerTask::set_terrain(RasterTerrain* _terrain) {
  terrain = _terrain;
  m_task.route_set_terrain(terrain);
}

fixed
GlideComputerTask::GetMacCready() const
{
  return m_task.GetMacCready();
}
