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

  // even if we are in auto mode, force the value in the computer.
  // if in auto mode, the value will get propagated out via the mechanism
  // below.
  GlidePolar glide_polar = task->get_glide_polar();
  glide_polar.set_mc(basic.MacCready);
  task->set_glide_polar(glide_polar);

  bool auto_updated = false;

  if (basic.Time != LastBasic().Time && !basic.gps.NAVWarning) {
    const AIRCRAFT_STATE current_as = ToAircraftState(Basic());
    const AIRCRAFT_STATE last_as = ToAircraftState(LastBasic());

    task->update(current_as, last_as);
    auto_updated = task->update_auto_mc(current_as, std::max(
                                          Calculated().LastThermalAverageSmooth, fixed_zero));
  }

  SetCalculated().task_stats = task->get_stats();
  SetCalculated().common_stats = task->get_common_stats();


/* JMW @todo 

   sending the risk mc to devices and global is
   disabled temporarily as this sets a feedback loop through devices,
   and the set mc call in ProcessBasicTask(), winding the mc down to
   zero

  SetMC(Calculated().common_stats.current_risk_mc);

  see ticket #583 for symptoms.

  this now also only changes mc if in auto mode and the computer has
  changed the value.  this may have a bearing on #498.
*/
  if (SettingsComputer().auto_mc && auto_updated) {
    // in auto mode, check for changes forced by the computer
    const fixed mc_computer = 
      task->get_glide_polar().get_mc();
    if (fabs(mc_computer-basic.MacCready)>fixed(0.01)) {
      SetMC(mc_computer);
    }
  }
}

void
GlideComputerTask::ProcessMoreTask()
{
  TerrainWarning();

  if (SettingsComputer().EnableBlockSTF)
    SetCalculated().V_stf = Calculated().common_stats.V_block;
  else
    SetCalculated().V_stf = Calculated().common_stats.V_dolphin;

  if (Calculated().task_stats.current_leg.solution_remaining.defined())
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
  GeoPoint null_point(Angle::native(fixed_zero), Angle::native(fixed_zero));
  const TaskStats& stats = Calculated().task_stats;
  const GlideResult& current = stats.current_leg.solution_remaining;

  TerrainIntersection its(null_point);

  if (!stats.task_valid) {
    g_terrain.set_max_range(fixed(max(fixed(20000.0), 
                                      MapProjection().GetScreenDistanceMeters())));
    its = g_terrain.find_intersection(state, polar);
  } else {
    its = g_terrain.find_intersection(state, current, polar);
  }

  SetCalculated().TerrainWarning = !its.out_of_range;
  if (!its.out_of_range)
    SetCalculated().TerrainWarningLocation = its.location;
}
