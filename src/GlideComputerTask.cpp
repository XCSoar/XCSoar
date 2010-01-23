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

#include "Task/TaskManager.hpp"

#include "GlideComputerTask.hpp"
#include "Protection.hpp"
#include "SettingsTask.hpp"
#include "Math/Earth.hpp"
#include "Math/Geometry.hpp"
#include "Math/LowPassFilter.hpp"
#include "RasterTerrain.h"
#include "GlideRatio.hpp"
#include "GlideTerrain.hpp"
#include "Logger.h"
#include "InputEvents.h"
#include "Components.hpp"
#include <assert.h>

#ifndef _MSC_VER
#include <algorithm>
using std::min;
using std::max;
#endif

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
GlideComputerTask::StartTask(const bool do_advance, const bool do_announce)
{
#ifdef OLD_TASK
  if (task.getActiveIndex()==0)
    task.advanceTaskPoint(SettingsComputer());
#endif
}


extern TaskBehaviour task_behaviour;

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


double
FAIFinishHeight(const SETTINGS_COMPUTER &settings,
                const DERIVED_INFO& Calculated, int wp)
{
#ifdef OLD_TASK
  int FinalWayPoint = task.getFinalWaypoint();
  if (wp == -1) {
    wp = FinalWayPoint;
  }
  double wp_alt;
  if(task.ValidTaskPoint(wp)) {
    wp_alt = way_points.get(task.getWaypointIndex(wp)).Altitude;
  } else {
    wp_alt = 0;
  }

  if (!task.TaskIsTemporary() && (wp == FinalWayPoint)) {
    if (task.getSettings().EnableFAIFinishHeight
        && !task.getSettings().AATEnabled) {
      return max(max((double)task.getSettings().FinishMinHeight,
          settings.SafetyAltitudeArrival) + wp_alt,
          Calculated.TaskStartAltitude-1000.0);
    } else {
      return max((double)task.getSettings().FinishMinHeight,
          settings.SafetyAltitudeArrival) + wp_alt;
    }
  } else {
    return wp_alt + settings.SafetyAltitudeArrival;
  }
#else 
  return 0;
#endif
}

double
GlideComputerTask::FAIFinishHeight(int wp) const
{
  return ::FAIFinishHeight(SettingsComputer(), Calculated(), wp);
}

bool
GlideComputerTask::InTurnSector(const int the_turnpoint) const
{
#ifdef OLD_TASK
  double AircraftBearing;

  if (!task.ValidTaskPoint(the_turnpoint))
    return false;

  if (task.getSettings().SectorType == 0) {
    if(Calculated().WaypointDistance < task.getSettings().SectorRadius)
      return true;

  } else if (task.getSettings().SectorType>0) {
    AircraftBearing = AngleLimit180(
      Bearing(task.getTaskPointLocation(the_turnpoint), Basic().Location)
      - task.getTaskPoint(the_turnpoint).Bisector);

    if (task.getSettings().SectorType == 2) {
      // JMW added german rules
      if (Calculated().WaypointDistance < 500)
        return true;
    }
    if ((AircraftBearing >= -45) && (AircraftBearing <= 45)) {
      if (task.getSettings().SectorType == 1) {
        if (Calculated().WaypointDistance < task.getSettings().SectorRadius)
          return true;

      } else {
        // JMW added german rules
        if(Calculated().WaypointDistance < 10000)
          return true;
      }
    }
  }
#endif
  return false;
}

bool
GlideComputerTask::ValidFinish() const
{
#ifdef OLD_TASK
  if ((task.getSettings().FinishMinHeight > 0)
      &&(Calculated().TerrainValid)
      &&(Calculated().AltitudeAGL < task.getSettings().FinishMinHeight))
    return false;
  else
    return true;
#else 
  return false;
#endif
}

bool
GlideComputerTask::InFinishSector(const int i)
{
#ifdef OLD_TASK
  double AircraftBearing;
  double FirstPointDistance;
  bool retval = false;

  if (!ValidFinish())
    return false;

  // Finish invalid
  if (!task.ValidTaskPoint(i))
    return false;

  // distance from aircraft to start point
  DistanceBearing(Basic().Location,
                  task.getTaskPointLocation(i),
                  &FirstPointDistance,
                  &AircraftBearing);

  bool InFinishSector = LastCalculated().InFinishSector;

  bool inrange = false;
  inrange = (FirstPointDistance<task.getSettings().FinishRadius);
  if (!inrange)
    InFinishSector = false;

  if (task.getSettings().FinishType == FINISH_CIRCLE) // Start Circle
  {
    retval = inrange;
    goto OnExit;
  }

  // Finish line
  AircraftBearing = AngleLimit180(AircraftBearing - task.getTaskPoint(i).InBound);

  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  if (task.getSettings().FinishType == FINISH_LINE)
    approaching = ((AircraftBearing >= -90) && (AircraftBearing <= 90));
  else
    // FAI 90 degree
    approaching = !((AircraftBearing >= 135) || (AircraftBearing <= -135));

  if (inrange) {
    if (InFinishSector) {
      // previously approaching the finish line
      if (!approaching) {
        // now moving away from finish line
        InFinishSector = false;
        retval = true;
        goto OnExit;
      }
    } else {
      if (approaching) {
        // now approaching the finish line
        InFinishSector = true;
      }
    }
  } else {
    InFinishSector = false;
  }

 OnExit:
  SetCalculated().InFinishSector = InFinishSector;
  return retval;
#else
  return false;
#endif
}

/*

  Track 'TaskStarted' in Calculated info, so it can be
  displayed in the task status dialog.

  Must be reset at start of flight.

  For multiple starts, after start has been passed, need
  to set the first waypoint to the start waypoint and
  then recalculate task stats.

*/

bool
GlideComputerTask::ValidStartSpeed(const DWORD Margin) const
{
#ifdef OLD_TASK
  if (task.getSettings().StartMaxSpeed == 0)
    return true;

  if (Basic().AirspeedAvailable) {
    if (Basic().IndicatedAirspeed > (task.getSettings().StartMaxSpeed + Margin))
      return false;
  } else {
    if (Basic().Speed > (task.getSettings().StartMaxSpeed + Margin))
      return false;
  }
#endif
  return true;
}

bool
GlideComputerTask::InsideStartHeight(const DWORD Margin) const
{
#ifdef OLD_TASK
  if (task.getSettings().StartMaxHeight == 0)
    return true;

  if (Calculated().TerrainValid)
    return true;

  if (task.getSettings().StartHeightRef == 0) {
    if (Calculated().AltitudeAGL > (task.getSettings().StartMaxHeight + Margin))
      return false;
  } else {
    if (Calculated().NavAltitude > (task.getSettings().StartMaxHeight + Margin))
      return false;
  }
#endif
  return true;
}

bool
GlideComputerTask::InStartSector_Internal(int Index,
					       double OutBound,
					       bool &LastInSector)
{
#ifdef OLD_TASK
  if (!way_points.verify_index(Index))
    return false;

  // No Task Loaded

  double AircraftBearing;
  double FirstPointDistance;

  // distance from aircraft to start point
  DistanceBearing(Basic().Location,
                  way_points.get(Index).Location,
                  &FirstPointDistance,
                  &AircraftBearing);

  bool inrange = false;
  inrange = (FirstPointDistance < task.getSettings().StartRadius);

  if (task.getSettings().StartType == START_CIRCLE)
    return inrange;

  // Start Line
  AircraftBearing = AngleLimit180(AircraftBearing - OutBound);

  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  if (task.getSettings().StartType == START_LINE) {
    // Start line
    approaching = ((AircraftBearing >= -90) && (AircraftBearing <= 90));
  } else {
    // FAI 90 degree
    approaching = ((AircraftBearing >= -45) && (AircraftBearing <= 45));
  }

  if (inrange) {
    return approaching;
  } else {
    // cheat fail of last because exited from side
    LastInSector = false;
  }
#endif
  return false;
}

bool
GlideComputerTask::InStartSector(bool *CrossedStart)
{
#ifdef OLD_TASK
  bool LastInStartSector = LastCalculated().InStartSector;

  bool isInSector = false;
  bool retval = false;

  if (!Calculated().Flying || !task.Valid())
    return false;

  int wp_index = task.getWaypointIndex(0);

  bool in_height = true;

  if ((task.getActiveIndex() > 0) && !task.ValidTaskPoint(task.getActiveIndex() + 1)) {
    // don't detect start if finish is selected
    retval = false;
    goto OnExit;
  }

  in_height = InsideStartHeight(task.getSettings().StartMaxHeightMargin);

  if ((wp_index != Calculated().StartSectorWaypoint)
      && (Calculated().StartSectorWaypoint >= 0)) {
    LastInStartSector = false;
    SetCalculated().StartSectorWaypoint = wp_index;
  }

  isInSector = in_height &
      InStartSector_Internal(wp_index, task.getTaskPoint(0).OutBound,
                             LastInStartSector);

  *CrossedStart = LastInStartSector && !isInSector;
  LastInStartSector = isInSector;
  if (*CrossedStart) {
    goto OnExit;
  }

  if (task.getSettings().EnableMultipleStartPoints) {
    for (int i = 0; i < MAXSTARTPOINTS; i++) {
      if (task_start_stats[i].Active && (task_start_points[i].Index>=0)
          && (task_start_points[i].Index != wp_index)) {

        retval = in_height & InStartSector_Internal(task_start_points[i].Index,
						    task_start_points[i].OutBound,
						    task_start_stats[i].InSector);
        isInSector |= retval;

        int index = task_start_points[i].Index;
        *CrossedStart = task_start_stats[i].InSector && !retval;
        task_start_stats[i].InSector = retval;
        if (*CrossedStart) {
          TASK_POINT tp = task.getTaskPoint(0);
          if (tp.Index != index) {
            tp.Index = index;
            task.setTaskPoint(0, tp);
            task.RefreshTask(SettingsComputer(), Basic());

            LastInStartSector = false;
            SetCalculated().StartSectorWaypoint = index;
          }
          goto OnExit;
        }
      }
    }
  }

 OnExit:
  SetCalculated().InStartSector = LastInStartSector;
  return isInSector;
#else
  return false;
#endif
}

bool
GlideComputerTask::ReadyToStart()
{
#ifdef OLD_TASK
  if (!Calculated().Flying)
    return false;

  if (task.getSettings().AutoAdvance == AUTOADVANCE_AUTO)
    return true;

  if ((task.getSettings().AutoAdvance == AUTOADVANCE_ARM)
      || (task.getSettings().AutoAdvance ==AUTOADVANCE_ARMSTART)) {
    if (task.isAdvanceArmed())
      return true;
  }
#endif
  return false;
}

bool
GlideComputerTask::ReadyToAdvance(bool reset, bool restart)
{
#ifdef OLD_TASK
  bool say_ready = false;

  SetCalculated().ActiveTaskPoint = task.getActiveIndex();

  if (!Calculated().Flying) {
    SetCalculated().ReadyWayPoint = -1;
    return false;
  }

  if (task.getSettings().AutoAdvance == AUTOADVANCE_AUTO) {
    if (reset)
      task.setAdvanceArmed(false);

    return true;
  }

  if (task.getSettings().AutoAdvance == AUTOADVANCE_ARM) {
    if (task.isAdvanceArmed()) {
      if (reset)
        task.setAdvanceArmed(false);

      return true;
    } else {
      say_ready = true;
    }
  }

  if (task.getSettings().AutoAdvance== AUTOADVANCE_ARMSTART) {
    if ((task.getActiveIndex() == 0) || restart) {
      if (!task.isAdvanceArmed()) {
        say_ready = true;
      } else if (reset) {
        task.setAdvanceArmed(false);
        return true;
      }
    } else {
      // JMW fixed 20070528
      if (task.getActiveIndex() >0) {
        if (reset)
          task.setAdvanceArmed(false);

        return true;
      }
    }
  }

  // see if we've gone back a waypoint (e.g. restart)
  if (task.getActiveIndex() < LastCalculated().ActiveTaskPoint) {
    SetCalculated().ReadyWayPoint = -1;
  }

  if (say_ready) {
    if ((int)task.getActiveIndex() != LastCalculated().ReadyWayPoint) {
      InputEvents::processGlideComputer(GCE_ARM_READY);
      SetCalculated().ReadyWayPoint = task.getActiveIndex();
    }
  }
#endif
  return false;
}

void
GlideComputerTask::CheckStart()
{
#ifdef OLD_TASK
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
#endif
}


void
GlideComputerTask::CheckRestart()
{
#ifdef OLD_TASK
  if((Basic().Time - Calculated().TaskStartTime < 3600)
     && (task.getActiveIndex() <= 1)) {
    CheckStart();
  }
#endif
}

void
GlideComputerTask::CheckFinish()
{
#ifdef OLD_TASK
  if (InFinishSector(task.getActiveIndex())) {
    SetCalculated().IsInSector = true;
    aatdistance.AddPoint(Basic().Location, task.getActiveIndex(),
        AATCloseDistance());

    if (!Calculated().ValidFinish) {
      SetCalculated().ValidFinish = true;
      AnnounceWayPointSwitch(false);
      SaveFinish();
    }
  }
#endif
}

void
GlideComputerTask::AddAATPoint(const unsigned taskwaypoint)
{
#ifdef OLD_TASK
  bool insector = false;
  if (taskwaypoint > 0) {
    if (task.getSettings().AATEnabled) {
      insector = task.InAATTurnSector(Basic().Location, taskwaypoint);
    } else {
      insector = InTurnSector(taskwaypoint);
    }

    if(insector) {
      if (taskwaypoint == task.getActiveIndex())
        SetCalculated().IsInSector = true;

      aatdistance.AddPoint(Basic().Location, taskwaypoint, AATCloseDistance());
    }
  }
#endif
}

void
GlideComputerTask::CheckInSector()
{
#ifdef OLD_TASK
  if (task.getActiveIndex()>0)
    AddAATPoint(task.getActiveIndex()-1);

  AddAATPoint(task.getActiveIndex());

  // JMW Start bug XXX

  if (aatdistance.HasEntered(task.getActiveIndex())) {
    if (ReadyToAdvance(true, false))
      AnnounceWayPointSwitch(true);

    if (Calculated().Flying)
      SetCalculated().ValidFinish = false;
  }
#endif
}

/**
 * Checks whether the current location is in a
 * turnpoint sector
 */
void
GlideComputerTask::InSector()
{
#ifdef OLD_TASK
  // Checks whether the active waypoint is valid
  if (!task.Valid())
    return;

  SetCalculated().IsInSector = false;

  if(task.getActiveIndex() == 0) {
    CheckStart();
  } else {
    if (task.ActiveIsFinalWaypoint()) {
      AddAATPoint(task.getActiveIndex() - 1);
      CheckFinish();
    } else {
      CheckRestart();
      if (task.getActiveIndex() > 0)
        CheckInSector();
    }
  }
#endif
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

double GlideComputerTask::SpeedHeight() {
#ifdef OLD_TASK
  if (Calculated().TaskDistanceToGo <= 0)
    return 0;

  // Fraction of task distance covered
  double d_fraction = Calculated().TaskDistanceCovered /
      (Calculated().TaskDistanceCovered + Calculated().TaskDistanceToGo);

  double dh_start = Calculated().TaskStartAltitude;
  double dh_finish = FAIFinishHeight(-1);

  // Excess height
  return Calculated().NavAltitude - (dh_start * (1.0 - d_fraction) + dh_finish * (d_fraction));
#else
  return 0;
#endif
}

#ifdef DEBUGTASKSPEED
void GlideComputerTask::DebugTaskCalculations()
{
  if ((Calculated().TaskStartTime>0)
      && (Basic().Time-Calculated().TaskStartTime>0)) {
      if (Calculated().Flying) {

        double effective_mc = EffectiveMacCready();
        DebugStore("%g %g %g %g %g %g %g %g %g %g %d %g %g # taskspeed\r\n",
                Basic().Time-Calculated().TaskStartTime,
                Calculated().TaskDistanceCovered,
                Calculated().TaskDistanceToGo,
                Calculated().TaskAltitudeRequired,
                Calculated().NavAltitude,
                Calculated().TaskSpeedAchieved,
                Calculated().TaskSpeed,
                Calculated().TaskSpeedInstantaneous,
                MACCREADY,
                effective_mc,
                task.getActiveIndex(),
                Calculated().DistanceVario,
                Calculated().GPSVario);
      }
    }
}
#endif


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

void
GlideComputerTask::ResetEnter()
{
#ifdef OLD_TASK
  aatdistance.ResetEnterTrigger(task.getActiveIndex());
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

