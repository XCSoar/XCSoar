/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include "Terrain/GlideTerrain.hpp"
#include "Components.hpp"

#include <algorithm>

using std::max;

// JMW TODO: abstract up to higher layer so a base copy of this won't
// call any event

GlideComputerTask::GlideComputerTask(ProtectedTaskManager &task): 
  GlideComputerBlackboard(task) {}

void
GlideComputerTask::ResetFlight(const bool full)
{
  m_task.reset();
}

void
GlideComputerTask::ProcessBasicTask()
{
  const NMEA_INFO &basic = Basic();

  ProtectedTaskManager::ExclusiveLease task(m_task);

  task->set_task_behaviour(SettingsComputer());

  if (!SettingsComputer().auto_mc) {
    GlidePolar glide_polar = task->get_glide_polar();
    glide_polar.set_mc(basic.MacCready);
    task->set_glide_polar(glide_polar);
  }

  if (basic.Time != LastBasic().Time && !basic.gps.NAVWarning) {
    const AIRCRAFT_STATE current_as = ToAircraftState(Basic());
    const AIRCRAFT_STATE last_as = ToAircraftState(LastBasic());

    task->update(current_as, last_as);
    task->update_auto_mc(current_as, std::max(
        Calculated().LastThermalAverageSmooth, fixed_zero));
  }

  SetCalculated().task_stats = task->get_stats();
  SetCalculated().common_stats = task->get_common_stats();
}

void
GlideComputerTask::ProcessMoreTask()
{
  SetMC(Calculated().common_stats.current_risk_mc);

  TerrainWarning();

  if (SettingsComputer().EnableBlockSTF)
    SetCalculated().V_stf = Calculated().common_stats.V_block;
  else
    SetCalculated().V_stf = Calculated().common_stats.V_dolphin;

  SetCalculated().AutoZoomDistance = 
    Calculated().task_stats.current_leg.solution_remaining.Vector.Distance;
}

void
GlideComputerTask::ProcessIdle()
{
  const AIRCRAFT_STATE as = ToAircraftState(Basic());
  ProtectedTaskManager::ExclusiveLease task(m_task);
  task->update_idle(as);
}

void
GlideComputerTask::TerrainWarning()
{
  if (terrain == NULL)
    return;

  const AIRCRAFT_STATE state = ToAircraftState(Basic());
  GlidePolar polar = m_task.get_glide_polar();

  GlideTerrain g_terrain(SettingsComputer(), *terrain);
  GeoPoint null_point;
  const TaskStats& stats = Calculated().task_stats;
  const GlideResult& current = stats.current_leg.solution_remaining;

  SetCalculated().TerrainWarningLocation = null_point;

  TerrainIntersection its(null_point);

  if (!stats.task_valid) {
    g_terrain.set_max_range(fixed(max(fixed(20000.0), 
                                      MapProjection().GetScreenDistanceMeters())));
    its = g_terrain.find_intersection(state, polar);
  } else {
    its = g_terrain.find_intersection(state, current, polar);
  }

  if (!its.out_of_range)
    SetCalculated().TerrainWarningLocation = its.location;
}
