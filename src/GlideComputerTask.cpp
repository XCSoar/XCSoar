/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "TaskClientCalc.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/GlideTerrain.hpp"
#include "Components.hpp"

#include <algorithm>

using std::max;

// JMW TODO: abstract up to higher layer so a base copy of this won't
// call any event


GlideComputerTask::GlideComputerTask(TaskClientCalc& task): 
  GlideComputerBlackboard(task) 
{

}


void
GlideComputerTask::ResetFlight(const bool full)
{
  m_task.reset();
}

void
GlideComputerTask::ProcessBasicTask()
{
  const NMEA_INFO &basic = Basic();

  m_task.set_task_behaviour(SettingsComputer());

  if (basic.Time != LastBasic().Time) {

    if (!basic.gps.NAVWarning) {
      const AIRCRAFT_STATE current_as = ToAircraftState(Basic());
      const AIRCRAFT_STATE last_as = ToAircraftState(LastBasic());

      m_task.update(current_as, last_as);
      m_task.update_auto_mc(current_as,
                            Calculated().AdjustedAverageThermal);
    }
  }

  SetCalculated().task_stats = m_task.get_stats();
  SetCalculated().common_stats = m_task.get_common_stats();

  TerrainWarning();

  if (SettingsComputer().EnableBlockSTF) {
    SetCalculated().V_stf = Calculated().common_stats.V_block;
  } else {
    SetCalculated().V_stf = Calculated().common_stats.V_dolphin;
  }

  SetCalculated().ZoomDistance = 
    Calculated().task_stats.current_leg.solution_remaining.Vector.Distance;

#ifdef OLD_TASK // target control
  if (!targetManipEvent.test()) {
    // don't calculate these if optimise function being invoked or
    // target is being adjusted
    LDNext();
  }
#endif
}

void
GlideComputerTask::ProcessIdle()
{
  const AIRCRAFT_STATE as = ToAircraftState(Basic());

  m_task.update_idle(as);
}


void
GlideComputerTask::TerrainWarning()
{
  const AIRCRAFT_STATE state = ToAircraftState(Basic());
  GlidePolar polar = m_task.get_glide_polar();

  terrain.Lock();
  GlideTerrain g_terrain(SettingsComputer(), terrain);
  GEOPOINT null_point;
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

  if (!its.out_of_range) {
    SetCalculated().TerrainWarningLocation = its.location;
  }

  terrain.Unlock();
}


void
GlideComputerTask::LDNext()
{
#ifdef OLD_TASK // LD calcs
  if (!task.Valid()) {
    SetCalculated().LDNext = INVALID_GR;
    SetCalculated().LDFinish = INVALID_GR;
    SetCalculated().GRFinish = INVALID_GR; // VENTA-ADDON

    return;
  }

  const double height_above_leg = Calculated().NavAltitude
      + Calculated().EnergyHeight - FAIFinishHeight(task.getActiveIndex());

  SetCalculated().LDNext = UpdateLD(Calculated().LDNext,
                                    Calculated().LegDistanceToGo,
                                    height_above_leg,
                                    0.5);

  const double final_height = FAIFinishHeight(-1);

  const double total_energy_height =
      Calculated().NavAltitude + Calculated().EnergyHeight;

  SetCalculated().LDFinish = UpdateLD(Calculated().LDFinish,
                                      Calculated().TaskDistanceToGo,
                                      total_energy_height-final_height,
                                      0.5);

  // VENTA-ADDON Classic geometric GR calculation without Total Energy
  /*
   * Paolo Ventafridda> adding a classic standard glide ratio
   * computation based on a geometric path with no total energy and
   * wind. This value is auto limited to a reasonable level which can
   * be useful during flight, currently 200. Over 200, you are no more
   * gliding to the final destination I am afraid, even on an ETA
   * . The infobox value has a decimal point if it is between 1 and
   * 99, otherwise it's a simple integer.
   */
  double GRsafecalc = Calculated().NavAltitude - final_height;
  if (GRsafecalc <= 0)
    SetCalculated().GRFinish = INVALID_GR;
  else {
    SetCalculated().GRFinish = Calculated().TaskDistanceToGo / GRsafecalc;
    if (Calculated().GRFinish > ALTERNATE_MAXVALIDGR || Calculated().GRFinish < 0)
      SetCalculated().GRFinish = INVALID_GR;
    else if (Calculated().GRFinish < 1)
      SetCalculated().GRFinish = 1;
  }
  // END VENTA-ADDON
#endif
}

/*
    // v1 = actual task speed achieved so far
    // d1 = distance travelled
    double konst;
    if (logger.isTaskDeclared()) {
      konst = 1.0;
    } else {
      konst = 1.1;
    }

    double termikLigaPoints = 0;
    if (d1 > 0) {
      termikLigaPoints = konst * (0.015 * 0.001 * d1 - (400.0 / (0.001 * d1)) + 12.0)
        * v1 * 3.6 * 100.0 / (double) SettingsComputer().Handicap;
    }

    SetCalculated().TermikLigaPoints = termikLigaPoints;
*/

/////////////////////////////////
