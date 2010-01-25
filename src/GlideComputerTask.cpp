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
#include "Task/TaskManager.hpp"
#include "RasterTerrain.h"
#include "Components.hpp"

#include <algorithm>

using std::max;

// JMW TODO: abstract up to higher layer so a base copy of this won't
// call any event

bool ForceFinalGlide= false;


GlideComputerTask::GlideComputerTask(TaskManager& task): 
  m_task(task) 
{

}


void
GlideComputerTask::ResetFlight(const bool full)
{
  terrain.Lock();
  m_task.reset();
  terrain.Unlock();
}

void
GlideComputerTask::ProcessBasicTask()
{
  if (Basic().Time != LastBasic().Time) {
    terrain.Lock();

  // JMW TODO OLD_TASK, this is a hack
    task_behaviour = SettingsComputer();
    task_behaviour.aat_min_time = 60*45;

//    task_behaviour.all_off();
//    task_behaviour.optimise_targets_range = true;
//  task_behaviour.auto_mc=true;
    task_behaviour.enable_olc = true;

    if (!Basic().NAVWarning) {
      m_task.update(Basic(), LastBasic());
    }
    terrain.Unlock();
  }

  SetCalculated().task_stats = m_task.get_stats();
  SetCalculated().common_stats = m_task.get_common_stats();

  if (SettingsComputer().EnableBlockSTF) {
    SetCalculated().V_stf = m_task.get_common_stats().V_block;
  } else {
    SetCalculated().V_stf = m_task.get_common_stats().V_dolphin;
  }

  SetCalculated().ZoomDistance = 
    Calculated().task_stats.current_leg.solution_remaining.Vector.Distance;

#ifdef OLD_TASK
  if (!targetManipEvent.test()) {
    // don't calculate these if optimise function being invoked or
    // target is being adjusted
    CheckTransitionFinalGlide();
    LDNext();
  }
#endif
}

void
GlideComputerTask::ProcessIdle()
{
  terrain.Lock();
  m_task.update_idle(Basic());
  terrain.Unlock();
}


void
GlideComputerTask::LDNext()
{
#ifdef OLD_TASK
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

void
GlideComputerTask::CheckForceFinalGlide()
{
#ifdef OLD_TASK
  // Auto Force Final Glide forces final glide display mode
  // if above final glide...
  if (task.isTaskAborted()) {
    ForceFinalGlide = false;
  } else {
    if (SettingsComputer().AutoForceFinalGlide) {
      if (!Calculated().FinalGlide) {
        if (Calculated().TaskAltitudeDifference>120)
          ForceFinalGlide = true;
        else
          ForceFinalGlide = false;

      } else {
        if (Calculated().TaskAltitudeDifference<-120)
          ForceFinalGlide = false;
        else
          ForceFinalGlide = true;
      }
    }
  }
#endif
}


void
GlideComputerTask::TerrainWarning()
{
#ifdef OLD_TASK
  if (!task.Valid()) {
    SetCalculated().TerrainWarningLocation.Latitude = 0.0;
    SetCalculated().TerrainWarningLocation.Longitude = 0.0;
    CheckFinalGlideThroughTerrain(0.0, 0.0);
    return;
  }

  CheckFinalGlideThroughTerrain(Calculated().LegDistanceToGo, Bearing(
      Basic().Location, task.getTargetLocation()));

  CheckForceFinalGlide();
#endif
}


void
GlideComputerTask::CheckTransitionFinalGlide()
{
#ifdef OLD_TASK
  if (!task.Valid()) {
    SetCalculated().FinalGlide = 0;
    return;
  }

  const unsigned FinalWayPoint = task.getFinalWaypoint();
  // update final glide mode status
  if ((task.getActiveIndex() == FinalWayPoint) || ForceFinalGlide) {
    if (Calculated().FinalGlide == 0)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE);

    SetCalculated().FinalGlide = 1;
  } else {
    if (Calculated().FinalGlide == 1)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);

    SetCalculated().FinalGlide = 0;
  }
#endif
}


double
GlideComputerTask::MacCreadyOrAvClimbRate(double this_maccready)
{
  double mc_val = this_maccready;
  bool is_final_glide = false;

#ifdef OLD_TASK
  if (Calculated().FinalGlide)
    is_final_glide = true;
#endif

  // when calculating 'achieved' task speed, need to use Mc if
  // not in final glide, or if in final glide mode and using
  // auto Mc, use the average climb rate achieved so far.

  if ((mc_val < 0.1)
      || (SettingsComputer().auto_mc
          && ((SettingsComputer().AutoMacCreadyMode == 0)
              || ((SettingsComputer().AutoMacCreadyMode == 2) && (is_final_glide))))) {

    mc_val = Calculated().AdjustedAverageThermal;
  }

  return max(0.1, mc_val);
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


void
GlideComputerTask::CheckFinalGlideThroughTerrain(double LegToGo, double LegBearing)
{
#ifdef OLD_TASK
  // Final glide through terrain updates
  if (Calculated().FinalGlide) {

    GEOPOINT loc;
    bool out_of_range;

    terrain.Lock();
    double distance_soarable =
      FinalGlideThroughTerrain(LegBearing,
                               Basic(), Calculated(),
                               SettingsComputer(), terrain,
                               &loc,
                               LegToGo, &out_of_range, NULL);
    terrain.Unlock();

    if ((!out_of_range)&&(distance_soarable< LegToGo)) {
      SetCalculated().TerrainWarningLocation = loc;
    } else {
      SetCalculated().TerrainWarningLocation.Latitude = 0.0;
      SetCalculated().TerrainWarningLocation.Longitude = 0.0;
    }
  } else {
    SetCalculated().TerrainWarningLocation.Latitude = 0.0;
    SetCalculated().TerrainWarningLocation.Longitude = 0.0;
  }
#endif
}


/**
 * Does the AutoMacCready calculations
 * @param mc_setting The old MacCready setting
 */
void
GlideComputerTask::DoAutoMacCready(double mc_setting)
{
#ifdef OLD_TASK
  bool is_final_glide = false;

  // if (AutoMacCready disabled) cancel calculation
  if (!SettingsComputer().auto_mc)
    return;

  double mc_new = mc_setting;
  static bool first_mc = true;

  // QUESTION TB: what about final glide around a turnpoint?
  if (Calculated().FinalGlide && task.ActiveIsFinalWaypoint()) {
    is_final_glide = true;
  } else {
    first_mc = true;
  }

  // if (not on Task)
  if (!task.Valid()) {
    if (Calculated().AdjustedAverageThermal>0) {
      // use the average climb speed of the last thermal
      mc_new = Calculated().AdjustedAverageThermal;
    }

  // if (on task, on final glide and activated at settings)
  } else if (((SettingsComputer().AutoMacCreadyMode == 0)
      || (SettingsComputer().AutoMacCreadyMode == 2)) && is_final_glide) {

    // QUESTION TB: time_remaining until what? and why 9000???
    double time_remaining = Basic().Time - Calculated().TaskStartTime - 9000;

    if (SettingsComputer().EnableOLC && (SettingsComputer().OLCRules == 0)
        && (Calculated().NavAltitude > Calculated().TaskStartAltitude)
        && (time_remaining > 0)) {

      mc_new = MacCreadyTimeLimit(Basic(), Calculated(),
          Calculated().WaypointBearing, time_remaining,
          Calculated().TaskStartAltitude);

    } else if (Calculated().TaskAltitudeDifference0 > 0) {

      // only change if above final glide with zero Mc
      // otherwise when we are well below, it will wind Mc back to
      // zero

      double slope = (Calculated().NavAltitude + Calculated().EnergyHeight
          - FAIFinishHeight(task.getActiveIndex()))
          / (Calculated().WaypointDistance + 1);

      double mc_pirker = PirkerAnalysis(Basic(), Calculated(),
          Calculated().WaypointBearing, slope);

      mc_pirker = max(0.0, mc_pirker);

      if (first_mc) {
        // don't allow Mc to wind down to zero when first achieving
        // final glide; but do allow it to wind down after that
        if (mc_pirker >= mc_new) {
          mc_new = mc_pirker;
          first_mc = false;
        } else if (SettingsComputer().AutoMacCreadyMode == 2) {
          // revert to averager based auto Mc
          if (Calculated().AdjustedAverageThermal > 0)
            mc_new = Calculated().AdjustedAverageThermal;
        }
      } else {
        mc_new = mc_pirker;
      }
    } else {
      // below final glide at zero Mc, never achieved final glide
      if (first_mc && (SettingsComputer().AutoMacCreadyMode == 2)) {
        // revert to averager based auto Mc
        if (Calculated().AdjustedAverageThermal > 0)
          mc_new = Calculated().AdjustedAverageThermal;
      }
    }
  } else if ((SettingsComputer().AutoMacCreadyMode == 1)
      || ((SettingsComputer().AutoMacCreadyMode == 2) && !is_final_glide)) {
    if (Calculated().AdjustedAverageThermal > 0)
      // use the average climb speed of the last thermal
      mc_new = Calculated().AdjustedAverageThermal;
  }

  // use a filter to prevent jumping of the MacCready setting
  GlidePolar::SetMacCready(LowPassFilter(mc_setting, mc_new, 0.15));
#endif
}


/////////////////////////////////


/*

  Track 'TaskStarted' in Calculated info, so it can be
  displayed in the task status dialog.

  Must be reset at start of flight.

  For multiple starts, after start has been passed, need
  to set the first waypoint to the start waypoint and
  then recalculate task stats.

*/

#ifdef OLD_TASK

void
GlideComputerTask::StartTask(const bool do_advance, const bool do_announce)
{
  if (task.getActiveIndex()==0)
    task.advanceTaskPoint(SettingsComputer());
}

bool
GlideComputerTask::ValidFinish() const
{
  if ((task.getSettings().FinishMinHeight > 0)
      &&(Calculated().TerrainValid)
      &&(Calculated().AltitudeAGL < task.getSettings().FinishMinHeight))
    return false;
  else
    return true;
}


bool
GlideComputerTask::ValidStartSpeed(const DWORD Margin) const
{
  if (task.getSettings().StartMaxSpeed == 0)
    return true;

  if (Basic().AirspeedAvailable) {
    if (Basic().IndicatedAirspeed > (task.getSettings().StartMaxSpeed + Margin))
      return false;
  } else {
    if (Basic().Speed > (task.getSettings().StartMaxSpeed + Margin))
      return false;
  }
  return true;
}

bool
GlideComputerTask::InsideStartHeight(const DWORD Margin) const
{
  if (task.getSettings().StartMaxHeight == 0)
    return true;

  if (Calculated().TerrainValid)
    return true;

  if (task.getSettings().StartHeightRef == 0) {
    if (Basic().AltitudeAGL > (task.getSettings().StartMaxHeight + Margin))
      return false;
  } else {
    if (Basic().NavAltitude > (task.getSettings().StartMaxHeight + Margin))
      return false;
  }
  return true;
}

void
GlideComputerTask::CheckStart()
{
  bool StartCrossed = false;

  if (InStartSector(&StartCrossed)) {
    SetCalculated().IsInSector = true;

    if (ReadyToStart()) {
      aatdistance.AddPoint(Basic().Location, 0, AATCloseDistance());
    }

    // TODO: we are ready to start even when outside start rules but
    // within margin
    if (ValidStartSpeed(task.getSettings().StartMaxSpeedMargin)) {
      ReadyToAdvance(false, true);
    }
    // TODO accuracy: monitor start speed throughout time in start sector
  }

  if (StartCrossed) {
    // TODO: Check whether speed and height are within the rules or
    // not (zero margin)
    if(!task.ActiveIsFinalWaypoint() && ValidStartSpeed() && InsideStartHeight()) {
      // This is set whether ready to advance or not, because it will
      // appear in the flight log, so if it's valid, it's valid.
      SetCalculated().ValidStart = true;

      if (ReadyToAdvance(true, true)) {
        task.setActiveIndex(0); // enforce this since it may be 1
        StartTask(true, true);
      }

      if (Calculated().Flying) {
        SetCalculated().ValidFinish = false;
      }
      // JMW TODO accuracy: This causes Vaverage to go bonkers
      // if the user has already passed the start
      // but selects the start

      // Note: pilot must have armed advance
      // for the start to be registered

      // ToLo: If speed and height are outside the rules they must be
      // within the margin...
    } else {
      if ((task.getActiveIndex()<=1)
          && !task.ActiveIsFinalWaypoint()
          && (Calculated().ValidStart==false)
          && (Calculated().Flying)) {

        // need to detect bad starts, just to get the statistics
        // in case the bad start is the best available, or the user
        // manually started
        StartTask(false, false);
        // Calculated().ValidStart = false;

        bool startTaskAnyway = false;

        if (ReadyToAdvance(true, true)) {

	  /* JMW TODO THIS IS BAD!!! SEND AN EVENT TO THE GUI INSTEAD
	     OF RUNNING A DIALOG FROM THE CALCULATIONS THREAD
          //DoStatusMessage(TEXT("Start Anyway?"));
          dlgStartTaskShowModal(&startTaskAnyway,
                                Calculated().TaskStartTime,
                                Calculated().TaskStartSpeed,
                                Calculated().TaskStartAltitude);
	  */
          if (startTaskAnyway) {
            task.setActiveIndex(0); // enforce this since it may be 1
            StartTask(true, true);
          }
        }

        SetCalculated().ValidStart = startTaskAnyway;

        if (Calculated().Flying) {
          SetCalculated().ValidFinish = false;
        }

        // TODO: Display infobox when only a bit over start rules
      }
    }
  }
}

#endif
