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

static Route &
GetPlannedRoute(ProtectedTaskManager &task)
{
  /* XXX hack - we need to move the Route of of TaskManager to resolve
     this */
  ProtectedTaskManager::ExclusiveLease lease(task);
  return lease->get_planned_route();
}

// JMW TODO: abstract up to higher layer so a base copy of this won't
// call any event

GlideComputerTask::GlideComputerTask(ProtectedTaskManager &task,
                                     ProtectedRoutePlanner &protected_route_planner,
                                     const RoutePlannerGlue &route_planner):
  GlideComputerRoute(protected_route_planner, route_planner,
                     GetPlannedRoute(task)),
  m_task(task)
{}

void
GlideComputerTask::Initialise()
{
}

void
GlideComputerTask::ResetFlight(const bool full)
{
  m_task.reset();
  GlideComputerRoute::ResetFlight();
}

void
GlideComputerTask::ProcessBasicTask()
{
  const MoreData &basic = Basic();
  DerivedInfo &derived = SetCalculated();

  ProtectedTaskManager::ExclusiveLease task(m_task);

  task->set_task_behaviour(SettingsComputer());

  if (time_advanced() && basic.location_available) {
    const AircraftState current_as = ToAircraftState(basic, Calculated());
    const AircraftState last_as = ToAircraftState(LastBasic(),
                                                   LastCalculated());

    task->update(current_as, last_as);

    if (Calculated().last_thermal.IsDefined()) {
      if (task->update_auto_mc(current_as, std::max(fixed_zero, 
                                                    Calculated().last_thermal_average_smooth))) {
        derived.auto_mac_cready = task->get_glide_polar().GetMC();
        derived.auto_mac_cready_available.Update(basic.clock);
      }
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
  GlidePolar glide_polar, safety_polar;

  {
    ProtectedTaskManager::Lease task(m_task);
    glide_polar = task->get_glide_polar();
    safety_polar = task->get_safety_polar();
  }

  GlideComputerRoute::ProcessRoute(Basic(), SetCalculated(), LastCalculated(),
                                   SettingsComputer(),
                                   glide_polar, safety_polar);

  if (SettingsComputer().EnableBlockSTF)
    SetCalculated().V_stf = Calculated().common_stats.V_block;
  else
    SetCalculated().V_stf = Calculated().common_stats.V_dolphin;

  if (Calculated().task_stats.current_leg.solution_remaining.IsDefined()) {
    const GeoVector &v = Calculated().task_stats.current_leg.solution_remaining.vector;
    SetCalculated().auto_zoom_distance = v.Distance;
  }
}

void
GlideComputerTask::ProcessIdle()
{
  const AircraftState as = ToAircraftState(Basic(), Calculated());
  ProtectedTaskManager::ExclusiveLease task(m_task);
  task->update_idle(as);
}

void 
GlideComputerTask::OnTakeoff()
{
  if (Calculated().altitude_agl_valid &&
      Calculated().altitude_agl > fixed(500))
    return;

  ProtectedTaskManager::ExclusiveLease task(m_task);
  task->takeoff_autotask(Basic().location, Calculated().terrain_altitude);
}

void 
GlideComputerTask::set_terrain(const RasterTerrain* _terrain) {
  GlideComputerRoute::set_terrain(_terrain);
}

fixed
GlideComputerTask::GetMacCready() const
{
  return m_task.GetMacCready();
}
