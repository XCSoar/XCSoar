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
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "NMEA/Aircraft.hpp"

#include <algorithm>

using std::max;

// JMW TODO: abstract up to higher layer so a base copy of this won't
// call any event

GlideComputerTask::GlideComputerTask(ProtectedTaskManager &task):
  GlideComputerBlackboard(task),
  terrain(NULL)
{}

void
GlideComputerTask::ResetFlight(const bool full)
{
  m_task.reset();
}

void
GlideComputerTask::ProcessBasicTask()
{
  const NMEA_INFO &basic = Basic();

  m_task.route_set_terrain(terrain);

  ProtectedTaskManager::ExclusiveLease task(m_task);

  task->set_task_behaviour(SettingsComputer());

  // even if we are in auto mode, force the value in the computer.
  // if in auto mode, the value will get propagated out via the mechanism
  // below.
  GlidePolar glide_polar = task->get_glide_polar();
  glide_polar.set_mc(basic.MacCready);
  task->set_glide_polar(glide_polar);

  bool auto_updated = false;

  if (basic.Time != LastBasic().Time && basic.LocationAvailable) {
    const AIRCRAFT_STATE current_as = ToAircraftState(basic, Calculated());
    const AIRCRAFT_STATE last_as = ToAircraftState(LastBasic(),
                                                   LastCalculated());

    task->update(current_as, last_as);
    auto_updated = task->update_auto_mc(current_as, std::max(
                                          Calculated().LastThermalAverageSmooth, fixed_zero));
  }

  SetCalculated().task_stats = task->get_stats();
  SetCalculated().common_stats = task->get_common_stats();
  SetCalculated().contest_stats = task->get_contest_stats();

  SetCalculated().glide_polar_task = task->get_glide_polar();
  SetCalculated().glide_polar_safety = task->get_safety_polar();

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
  m_task.route_update_polar(Basic().wind);
  SetCalculated().glide_polar_reach = m_task.get_reach_polar();

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
  {
    ProtectedTaskManager::ExclusiveLease task(m_task);
    task->update_idle(as);
  }

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
      m_task.route_solve(dest, start, h_ceiling);
      SetCalculated().TerrainWarning = m_task.intersection(start, dest,
                                                           SetCalculated().TerrainWarningLocation);
      return;
    } else {
      m_task.route_solve(start, start, h_ceiling);
    }
  }
  SetCalculated().TerrainWarning = false;
}

void
GlideComputerTask::TerrainWarning()
{
  if (SettingsComputer().FinalGlideTerrain && terrain) {
    // @todo: update TerrainBase in new footprint calculations,
    // remove TerrainFootprint function from GlideComputerAirData

    const AIRCRAFT_STATE state = ToAircraftState(Basic(), Calculated());
    const AGeoPoint start (state.get_location(), state.NavAltitude);
    m_task.solve_reach(start);
    SetCalculated().TerrainBase = fixed(m_task.get_terrain_base());
  } else {
    // fallback to current terrain altitude
    SetCalculated().TerrainBase = Calculated().TerrainAlt;
  }
}

void 
GlideComputerTask::OnTakeoff()
{
  ProtectedTaskManager::ExclusiveLease task(m_task);
  task->takeoff_autotask(Basic().Location);
}
