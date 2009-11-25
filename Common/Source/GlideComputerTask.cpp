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
#include "Protection.hpp"
#include "Task.h"
#include "SettingsTask.hpp"
#include "WayPoint.hpp"
#include "Math/Earth.hpp"
#include "Math/Geometry.hpp"
#include "Math/Pressure.h"
#include "Math/LowPassFilter.hpp"
#include "McReady.h"
#include "GlideRatio.hpp"
#include "GlideSolvers.hpp"
#include "Logger.h"
#include "InputEvents.h"
#include "Components.hpp"
#include "WayPointList.hpp"
#include <assert.h>
#include "TaskVisitor.hpp"

#ifndef _MSC_VER
#include <algorithm>
using std::min;
using std::max;
#endif

// JMW TODO: abstract up to higher layer so a base copy of this won't
// call any event

bool  ForceFinalGlide= false;

void GlideComputerTask::ResetFlight(const bool full)
{
  if (full) {
    olc.ResetFlight();
    aatdistance.Reset();
  }
  SetCalculated().BestAlternate = -1;
}


void GlideComputerTask::StartTask(const bool do_advance,
				  const bool do_announce)
{
  if (task.getActiveIndex()==0) {
    task.advanceTaskPoint(SettingsComputer());
  }
}


void GlideComputerTask::ProcessBasicTask(const double mc,
					 const double ce)
{
  DistanceToHome();
  DistanceToNext();
  AltitudeRequired(mc, ce);
  if (!targetManipEvent.test()) {
    // don't calculate these if optimise function being invoked or
    // target is being adjusted
    CheckTransitionFinalGlide();
    DistanceCovered();
    TaskStatistics(mc, ce);
    AATStats();
    LegSpeed();
    LDNext();
    TaskSpeed(mc, ce);
  }
}

void
GlideComputerTask::ProcessIdle()
{
  if (task.isTaskAborted()) {
    SortLandableWaypoints();
  }
  DoBestAlternateSlow();
}

/**
 * Logs GPS fixes for OLC optimization
 * @return True is new fix is saved, False otherwise
 */
bool GlideComputerTask::DoLogging() {
  if (Calculated().Flying && olc_clock.check_advance(Basic().Time)) {
    bool restart = olc.addPoint(Basic().Location,
				Calculated().NavAltitude,
				Calculated().WaypointBearing,
				Basic().Time-Calculated().TakeOffTime,
				SettingsComputer());

    if (restart && SettingsComputer().EnableOLC) {
      SetCalculated().ValidFinish = false;
      StartTask(false, false);
      SetCalculated().ValidStart = true;
    }
    return true;
  } else {
    return false;
  }
}

// VENTA3 added radial
void GlideComputerTask::DistanceToHome() {

  if (!way_points.verify_index(SettingsComputer().HomeWaypoint)) {
    SetCalculated().HomeDistance = 0.0;
    SetCalculated().HomeRadial = 0.0; // VENTA3
  } else {
    DistanceBearing(way_points.get(SettingsComputer().HomeWaypoint).Location,
                    Basic().Location,
                    &SetCalculated().HomeDistance,
                    &SetCalculated().HomeRadial);
  }
}

void GlideComputerTask::DistanceToNext()
{
  if(task.Valid()) {

    DistanceBearing(Basic().Location,
                    task.getActiveLocation(),
                    &SetCalculated().WaypointDistance,
                    &SetCalculated().WaypointBearing);

    SetCalculated().ZoomDistance = Calculated().WaypointDistance;

    if ((Calculated().IsInSector)
        && (task.getActiveIndex()==0)
        && task.ValidTaskPoint(1)
        && !task.TaskIsTemporary()) {

      // JMW set waypoint bearing to start direction if in start sector
      SetCalculated().WaypointBearing = Bearing(Basic().Location,
                                                task.getTargetLocation(1));
    } else {

      DistanceBearing(Basic().Location,
                      task.getTargetLocation(),
                      &SetCalculated().WaypointDistance,
                      &SetCalculated().WaypointBearing);

      if (Calculated().WaypointDistance>AATCloseDistance()*3.0) {
        SetCalculated().ZoomDistance = max(Calculated().WaypointDistance,
                                           Calculated().ZoomDistance);
      } else {
        SetCalculated().WaypointBearing = AATCloseBearing();
      }
    }
  } else {
    SetCalculated().ZoomDistance = 0;
    SetCalculated().WaypointDistance = 0;
    SetCalculated().WaypointBearing = 0;
  }
}

void GlideComputerTask::AltitudeRequired(const double this_maccready,
					 const double cruise_efficiency)
{
  if(task.Valid()) {
    double wp_alt = FAIFinishHeight(task.getActiveIndex());
    double height_above_wp =
      Calculated().NavAltitude + Calculated().EnergyHeight
      - wp_alt;

    SetCalculated().NextAltitudeRequired =
      GlidePolar::MacCreadyAltitude(this_maccready,
                                    Calculated().WaypointDistance,
                                    Calculated().WaypointBearing,
                                    Calculated().WindSpeed, Calculated().WindBearing,
                                    0, 0,
                                    true,
                                    NULL, height_above_wp, cruise_efficiency
        );
    // JMW CHECK FGAMT

    // VENTA6
    if (this_maccready==0 )
      SetCalculated().NextAltitudeRequired0=Calculated().NextAltitudeRequired;
    else
      SetCalculated().NextAltitudeRequired0 =
        GlidePolar::MacCreadyAltitude(0,
                                      Calculated().WaypointDistance,
                                      Calculated().WaypointBearing,
                                      Calculated().WindSpeed, Calculated().WindBearing,
                                      0, 0,
                                      true,
                                      NULL, height_above_wp, cruise_efficiency
				);

      SetCalculated().NextAltitudeRequired += wp_alt;
      SetCalculated().NextAltitudeRequired0 += wp_alt; // VENTA6

      SetCalculated().NextAltitudeDifference =
        Calculated().NavAltitude
        + Calculated().EnergyHeight
        - Calculated().NextAltitudeRequired;

      SetCalculated().NextAltitudeDifference0 =
        Calculated().NavAltitude
        + Calculated().EnergyHeight
        - Calculated().NextAltitudeRequired0;
  } else {
    SetCalculated().NextAltitudeRequired = 0;
    SetCalculated().NextAltitudeDifference = 0;
    SetCalculated().NextAltitudeDifference0 = 0; // VENTA6
  }
}

double GlideComputerTask::AATCloseBearing() const
{
  // ensure waypoint goes in direction of track if very close
  TASK_POINT tp = task.getTaskPoint();
  double course_bearing = Bearing(task.getActiveLocation(), Basic().Location)
    +tp.AATTargetOffsetRadial;
  return AngleLimit360(course_bearing);
}

double
FAIFinishHeight(const SETTINGS_COMPUTER &settings,
		const DERIVED_INFO& Calculated, int wp)
{
  int FinalWayPoint = task.getFinalWaypoint();
  if (wp== -1) {
    wp = FinalWayPoint;
  }
  double wp_alt;
  if(task.ValidTaskPoint(wp)) {
    wp_alt = way_points.get(task.getWaypointIndex(wp)).Altitude;
  } else {
    wp_alt = 0;
  }

  if (!task.TaskIsTemporary() && (wp==FinalWayPoint)) {
    if (task.getSettings().EnableFAIFinishHeight
        && !task.getSettings().AATEnabled) {
      return max(max((double)task.getSettings().FinishMinHeight,
		     settings.SAFETYALTITUDEARRIVAL)+ wp_alt,
                 Calculated.TaskStartAltitude-1000.0);
    } else {
      return max((double)task.getSettings().FinishMinHeight,
		 settings.SAFETYALTITUDEARRIVAL)+wp_alt;
    }
  } else {
    return wp_alt + settings.SAFETYALTITUDEARRIVAL;
  }
}

double GlideComputerTask::FAIFinishHeight(int wp) const
{
  return ::FAIFinishHeight(SettingsComputer(), Calculated(), wp);
}

bool GlideComputerTask::InTurnSector(const int the_turnpoint) const
{
  double AircraftBearing;

  if (!task.ValidTaskPoint(the_turnpoint)) return false;

  if(task.getSettings().SectorType==0) {
    if(Calculated().WaypointDistance < task.getSettings().SectorRadius) {
      return true;
    }
  } else if (task.getSettings().SectorType>0) {
    AircraftBearing = AngleLimit180(
      Bearing(task.getTaskPointLocation(the_turnpoint),
              Basic().Location)
      - task.getTaskPoint(the_turnpoint).Bisector);

    if (task.getSettings().SectorType==2) {
      // JMW added german rules
      if (Calculated().WaypointDistance<500) {
        return true;
      }
    }
    if( (AircraftBearing >= -45) && (AircraftBearing <= 45)) {
      if (task.getSettings().SectorType==1) {
        if(Calculated().WaypointDistance < task.getSettings().SectorRadius) {
          return true;
        }
      } else {
        // JMW added german rules
        if(Calculated().WaypointDistance < 10000) {
          return true;
        }
      }
    }
  }
  return false;
}

bool GlideComputerTask::ValidFinish( ) const
{
  if ((task.getSettings().FinishMinHeight>0)
      &&(Calculated().TerrainValid)
      &&(Calculated().AltitudeAGL<task.getSettings().FinishMinHeight)) {
    return false;
  } else {
    return true;
  }
}

bool GlideComputerTask::InFinishSector(const int i)
{
  double AircraftBearing;
  double FirstPointDistance;
  bool retval = false;

  if (!ValidFinish()) return false;

  // Finish invalid
  if (!task.ValidTaskPoint(i)) return false;

  // distance from aircraft to start point
  DistanceBearing(Basic().Location,
                  task.getTaskPointLocation(i),
                  &FirstPointDistance,
                  &AircraftBearing);

  bool InFinishSector = LastCalculated().InFinishSector;

  bool inrange = false;
  inrange = (FirstPointDistance<task.getSettings().FinishRadius);
  if (!inrange) {
    InFinishSector = false;
  }

  if(task.getSettings().FinishType == FINISH_CIRCLE) // Start Circle
    {
      retval = inrange;
      goto OnExit;
    }

  // Finish line
  AircraftBearing = AngleLimit180(AircraftBearing - task.getTaskPoint(i).InBound);

  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  if(task.getSettings().FinishType==FINISH_LINE) {
    approaching = ((AircraftBearing >= -90) && (AircraftBearing <= 90));
  } else {
    // FAI 90 degree
    approaching = !((AircraftBearing >= 135) || (AircraftBearing <= -135));
  }

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
}

/*

  Track 'TaskStarted' in Calculated info, so it can be
  displayed in the task status dialog.

  Must be reset at start of flight.

  For multiple starts, after start has been passed, need
  to set the first waypoint to the start waypoint and
  then recalculate task stats.

*/

bool GlideComputerTask::ValidStartSpeed(const DWORD Margin) const
{
  bool valid = true;
  if (task.getSettings().StartMaxSpeed!=0) {
    if (Basic().AirspeedAvailable) {
      if (Basic().IndicatedAirspeed>(task.getSettings().StartMaxSpeed+Margin))
        valid = false;
    } else {
      if (Basic().Speed>(task.getSettings().StartMaxSpeed+Margin))
        valid = false;
    }
  }
  return valid;
}

bool GlideComputerTask::InsideStartHeight(const DWORD Margin) const
{
  bool valid = true;
  if ((task.getSettings().StartMaxHeight!=0)&&(Calculated().TerrainValid)) {
    if (task.getSettings().StartHeightRef == 0) {
      if (Calculated().AltitudeAGL>(task.getSettings().StartMaxHeight+Margin))
	valid = false;
    } else {
      if (Calculated().NavAltitude>(task.getSettings().StartMaxHeight+Margin))
	valid = false;
    }
  }
  return valid;
}

bool GlideComputerTask::InStartSector_Internal(int Index,
					       double OutBound,
					       bool &LastInSector)
{
  if (!way_points.verify_index(Index)) return false;

  // No Task Loaded

  double AircraftBearing;
  double FirstPointDistance;

  // distance from aircraft to start point
  DistanceBearing(Basic().Location,
                  way_points.get(Index).Location,
                  &FirstPointDistance,
                  &AircraftBearing);

  bool inrange = false;
  inrange = (FirstPointDistance<task.getSettings().StartRadius);

  if(task.getSettings().StartType==START_CIRCLE) {
    return inrange;
  }

  // Start Line
  AircraftBearing = AngleLimit180(AircraftBearing - OutBound);

  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  if(task.getSettings().StartType==START_LINE) { // Start line
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

  return false;
}

bool GlideComputerTask::InStartSector(bool *CrossedStart)
{
  bool LastInStartSector = LastCalculated().InStartSector;

  bool isInSector= false;
  bool retval=false;

  if (!Calculated().Flying ||
      !task.Valid())
    return false;

  int wp_index = task.getWaypointIndex(0);

  bool in_height = true;

  if ((task.getActiveIndex()>0)
      && !task.ValidTaskPoint(task.getActiveIndex()+1)) {
    // don't detect start if finish is selected
    retval = false;
    goto OnExit;
  }

  in_height = InsideStartHeight(task.getSettings().StartMaxHeightMargin);

  if ((wp_index != Calculated().StartSectorWaypoint)
      && (Calculated().StartSectorWaypoint>=0)) {
    LastInStartSector = false;
    SetCalculated().StartSectorWaypoint = wp_index;
  }

  isInSector = in_height & InStartSector_Internal(wp_index,
						  task.getTaskPoint(0).OutBound,
						  LastInStartSector);

  *CrossedStart = LastInStartSector && !isInSector;
  LastInStartSector = isInSector;
  if (*CrossedStart) {
    goto OnExit;
  }

  if (task.getSettings().EnableMultipleStartPoints) {
    for (int i=0; i<MAXSTARTPOINTS; i++) {
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
            task.setTaskPoint(0,tp);
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
}

bool GlideComputerTask::ReadyToStart() {
  if (!Calculated().Flying) {
    return false;
  }
  if (task.getSettings().AutoAdvance== AUTOADVANCE_AUTO) {
    return true;
  }
  if ((task.getSettings().AutoAdvance== AUTOADVANCE_ARM)
      || (task.getSettings().AutoAdvance==AUTOADVANCE_ARMSTART)) {
    if (task.isAdvanceArmed()) {
      return true;
    }
  }
  return false;
}

bool GlideComputerTask::ReadyToAdvance(bool reset, bool restart) {
  bool say_ready = false;

  // 0: Manual
  // 1: Auto
  // 2: Arm
  // 3: Arm start

  SetCalculated().ActiveTaskPoint = task.getActiveIndex();

  if (!Calculated().Flying) {
    SetCalculated().ReadyWayPoint = -1;
    return false;
  }

  if (task.getSettings().AutoAdvance== AUTOADVANCE_AUTO) {
    if (reset) {
      task.setAdvanceArmed(false);
    }
    return true;
  }
  if (task.getSettings().AutoAdvance== AUTOADVANCE_ARM) {
    if (task.isAdvanceArmed()) {
      if (reset) {
        task.setAdvanceArmed(false);
      }
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
        if (reset) {
          task.setAdvanceArmed(false);
        }
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
  return false;
}

void GlideComputerTask::CheckStart() {
  bool StartCrossed= false;

  if (InStartSector(&StartCrossed)) {
    SetCalculated().IsInSector = true;

    if (ReadyToStart()) {
      aatdistance.AddPoint(Basic().Location,
			   0,
			   AATCloseDistance());
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
    if(!task.ActiveIsFinalWaypoint()
       && ValidStartSpeed() && InsideStartHeight()) {

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
//        Calculated().ValidStart = false;

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


void GlideComputerTask::CheckRestart()
{
  if((Basic().Time - Calculated().TaskStartTime < 3600)
     &&(task.getActiveIndex()<=1)) {
    CheckStart();
  }
}

void GlideComputerTask::CheckFinish() {
  if (InFinishSector(task.getActiveIndex())) {
    SetCalculated().IsInSector = true;
    aatdistance.AddPoint(Basic().Location,
                         task.getActiveIndex(),
			 AATCloseDistance());
    if (!Calculated().ValidFinish) {
      SetCalculated().ValidFinish = true;
      AnnounceWayPointSwitch(false);
      SaveFinish();
    }
  }
}

void GlideComputerTask::AddAATPoint(const unsigned taskwaypoint) {
  bool insector = false;
  if (taskwaypoint>0) {
    if (task.getSettings().AATEnabled) {
      insector = task.InAATTurnSector(Basic().Location, taskwaypoint);
    } else {
      insector = InTurnSector(taskwaypoint);
    }
    if(insector) {
      if (taskwaypoint == task.getActiveIndex()) {
        SetCalculated().IsInSector = true;
      }
      aatdistance.AddPoint(Basic().Location,
                           taskwaypoint,
			   AATCloseDistance());
    }
  }
}

void GlideComputerTask::CheckInSector() {

  if (task.getActiveIndex()>0) {
    AddAATPoint(task.getActiveIndex()-1);
  }
  AddAATPoint(task.getActiveIndex());

  // JMW Start bug XXX

  if (aatdistance.HasEntered(task.getActiveIndex())) {
    if (ReadyToAdvance(true, false)) {
      AnnounceWayPointSwitch(true);
    }
    if (Calculated().Flying) {
      SetCalculated().ValidFinish = false;
    }
  }
}

/**
 * Checks whether the current location is in a
 * turnpoint sector
 */
void GlideComputerTask::InSector()
{
  // Checks whether the active waypoint is valid
  if (!task.Valid()) return;

  SetCalculated().IsInSector = false;

  if(task.getActiveIndex() == 0) {
    CheckStart();
  } else {
    if(task.ActiveIsFinalWaypoint()) {
      AddAATPoint(task.getActiveIndex()-1);
      CheckFinish();
    } else {
      CheckRestart();
      if (task.getActiveIndex()>0) {
        CheckInSector();
      }
    }
  }
}

void GlideComputerTask::LDNext() {
  if (!task.Valid()) {
    SetCalculated().LDNext = INVALID_GR;
    SetCalculated().LDFinish = INVALID_GR;
    SetCalculated().GRFinish = INVALID_GR; // VENTA-ADDON

    return;
  }

  const double height_above_leg = Calculated().NavAltitude+Calculated().EnergyHeight
    - FAIFinishHeight(task.getActiveIndex());

  SetCalculated().LDNext = UpdateLD(Calculated().LDNext,
                                    Calculated().LegDistanceToGo,
                                    height_above_leg,
                                    0.5);

  const double final_height = FAIFinishHeight(-1);

  const double total_energy_height = Calculated().NavAltitude + Calculated().EnergyHeight;

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
  if (GRsafecalc <=0)
    SetCalculated().GRFinish = INVALID_GR;
  else {
    SetCalculated().GRFinish = Calculated().TaskDistanceToGo / GRsafecalc;
    if ( Calculated().GRFinish >ALTERNATE_MAXVALIDGR || Calculated().GRFinish <0 )
      SetCalculated().GRFinish = INVALID_GR;
    else
      if ( Calculated().GRFinish <1 )
        SetCalculated().GRFinish = 1;
  }
  // END VENTA-ADDON
}

void GlideComputerTask::CheckForceFinalGlide() {
  // Auto Force Final Glide forces final glide display mode
  // if above final glide...
  if (task.isTaskAborted()) {
    ForceFinalGlide = false;
  } else {
    if (SettingsComputer().AutoForceFinalGlide) {
      if (!Calculated().FinalGlide) {
        if (Calculated().TaskAltitudeDifference>120) {
          ForceFinalGlide = true;
        } else {
          ForceFinalGlide = false;
        }
      } else {
        if (Calculated().TaskAltitudeDifference<-120) {
          ForceFinalGlide = false;
        } else {
          ForceFinalGlide = true;
        }
      }
    }
  }
}

void GlideComputerTask::LegSpeed()
{
  if (!task.Valid() || !task.ValidTaskPoint(1)) {
    SetCalculated().LegSpeed = 0;
    return;
  }

  if (Basic().Time > Calculated().LegStartTime) {
    SetCalculated().LegSpeed = Calculated().LegDistanceCovered
      / (Basic().Time - Calculated().LegStartTime);
  } else if (Basic().Time< Calculated().LegStartTime) {
    SetLegStart();
  }
}

void GlideComputerTask::TerrainWarning()
{
  if (!task.Valid()) {
    SetCalculated().TerrainWarningLocation.Latitude = 0.0;
    SetCalculated().TerrainWarningLocation.Longitude = 0.0;
    CheckFinalGlideThroughTerrain(0.0, 0.0);
    return;
  }

  CheckFinalGlideThroughTerrain(Calculated().LegDistanceToGo,
                                Bearing(Basic().Location,
                                        task.getTargetLocation()));
  CheckForceFinalGlide();
}

void GlideComputerTask::DistanceCovered()
{
  if (!task.Valid()) {
    SetCalculated().LegDistanceCovered = 0;
    SetCalculated().TaskDistanceCovered = 0;
  }
  double LegCovered;

  if ((task.getActiveIndex()==0) || task.TaskIsTemporary()) {
    LegCovered = 0;
  } else {
    GEOPOINT w0 = task.getTargetLocation(task.getActiveIndex()-1);
    GEOPOINT w1 = task.getTargetLocation();
    // TODO accuracy: Get best range point to here...

    LegCovered = ProjectedDistance(w0, w1, Basic().Location);

    if ((task.getSettings().StartType==START_CIRCLE) && (task.getActiveIndex()==1)) {
      // Correct speed calculations for radius
      // JMW TODO accuracy: legcovered replace this with more accurate version
      // LegDistance -= StartRadius;
      LegCovered = max(0.0, LegCovered - task.getSettings().StartRadius);
    }
  }
  SetCalculated().LegDistanceCovered = LegCovered;
  SetCalculated().TaskDistanceCovered = LegCovered;

  // Now add distances for start to previous waypoint

  if (!task.TaskIsTemporary() && (task.getActiveIndex()>0)) {
    if (!task.getSettings().AATEnabled) {
      for (unsigned i=0; i+1< task.getActiveIndex(); i++) {
        SetCalculated().TaskDistanceCovered +=
          task.getTaskPoint(i+1).LegDistance;
      }
    } else {
      // JMW added correction for distance covered
      SetCalculated().TaskDistanceCovered =
        aatdistance.DistanceCovered(Basic().Location,
                                    task.getActiveIndex(),
				    AATCloseDistance());
    }
  }
}

class TaskStatisticsVisitor:
 public RelativeTaskLegVisitor
{
public:
  TaskStatisticsVisitor(const NMEA_INFO &_gps_info,
                        DERIVED_INFO &_calculated_info,
                        const double _maccready,
                        const double _ce,
                        const double _finishHeight,
                        const double _closedistance,
                        const double _closebearing):
    gps_info(_gps_info),
    calculated_info(_calculated_info),
    maccready(_maccready),
    cruise_efficiency(_ce),
    finishHeight(_finishHeight),
    closedistance(_closedistance),
    closebearing(_closebearing)
    {

    };

  void visit_reset()
    {
      activeIndex = _task->getActiveIndex();
      if (_task->getSettings().AATEnabled
          && (_task->getActiveIndex()>0)
          && (_task->ValidTaskPoint(activeIndex+1))
          && calculated_info.IsInSector
          && !task.TaskIsTemporary())
      {
        in_aat_sector = true;
      } else {
        in_aat_sector = false;
      }

      if (in_aat_sector && (maccready>0.1)) {
        calc_turning_now = true;
      } else {
        calc_turning_now = false;
      }

      // accumulators

      calculated_info.TaskDistanceToGo = 0;
      calculated_info.TaskTimeToGo = 0;
      calculated_info.TaskTimeToGoTurningNow = 0;
      TaskAltitudeRequired = 0;
      TaskAltitudeRequired0 = 0;
      FinalWayPoint = _task->getFinalWaypoint();
      height_above_finish = calculated_info.NavAltitude+
        calculated_info.EnergyHeight-finishHeight;
      StartBestCruiseTrack = 0;

    };

  void visit_null()
    {
      calculated_info.LegTimeToGo = 0;
      calculated_info.TaskTimeToGo = 0;
      calculated_info.TaskTimeToGoTurningNow = -1;
      if (!_task->getSettings().AATEnabled) {
        calculated_info.AATTimeToGo = 0;
      }

      calculated_info.LegDistanceToGo = 0;
      calculated_info.TaskDistanceToGo = 0;

      //    Calculated().TaskSpeed = 0;

      calculated_info.TaskAltitudeRequired = 0;
      calculated_info.TaskAltitudeDifference = 0;
      calculated_info.TaskAltitudeDifference0 = 0;

      // no task selected, so work things out at current heading

      GlidePolar::MacCreadyAltitude(maccready, 100.0,
                                    gps_info.TrackBearing,
                                    calculated_info.WindSpeed,
                                    calculated_info.WindBearing,
                                    &(calculated_info.BestCruiseTrack),
                                    &(calculated_info.VMacCready),
                                    (calculated_info.FinalGlide==1),
                                    NULL, 1.0e6, cruise_efficiency);
    };

  void visit_single(TASK_POINT &point0, const unsigned index0)
    {

    };
  void visit_leg_before(TASK_POINT &point0, const unsigned index0,
                        TASK_POINT &point1, const unsigned index1)
    {
      // nothing
    };
  void visit_leg_current(TASK_POINT &point0, const unsigned index0,
                         TASK_POINT &point1, const unsigned index1)
    {
      if (index1==0) {
        addLeg(_task->getTargetLocation(index1),
               _task->getTargetLocation(index1), true, false);

        if (calculated_info.IsInSector && !_task->TaskIsTemporary()) {
          // set best cruise track to first leg bearing when in start sector
          calculated_info.BestCruiseTrack = StartBestCruiseTrack;
        }

      } else {
        addLeg(gps_info.Location,
               _task->getTargetLocation(index1), true, false);
      }
      finish();
    }

  void finish() {
    if (calc_turning_now) {
      calculated_info.TaskTimeToGoTurningNow +=
        gps_info.Time-calculated_info.TaskStartTime;
    } else {
      calculated_info.TaskTimeToGoTurningNow = -1;
    }

    double total_energy_height = calculated_info.NavAltitude
      + calculated_info.EnergyHeight;

    calculated_info.TaskAltitudeRequired = TaskAltitudeRequired + finishHeight;

    TaskAltitudeRequired0 += finishHeight;

    calculated_info.TaskAltitudeDifference = total_energy_height
      - calculated_info.TaskAltitudeRequired;

    calculated_info.TaskAltitudeDifference0 = total_energy_height
      - TaskAltitudeRequired0;

    calculated_info.NextAltitudeDifference0 = total_energy_height
      - calculated_info.NextAltitudeRequired0;

  };

  void visit_leg_after(TASK_POINT &point0, const unsigned index0,
                       TASK_POINT &point1, const unsigned index1)
    {
      if (_task->TaskIsTemporary()) {
        return;
      }
      addLeg(_task->getTargetLocation(index0),
             _task->getTargetLocation(index1), false,
             index0==activeIndex);

      if (index1==1) {
        StartBestCruiseTrack = Bearing(_task->getTargetLocation(index0),
                                       _task->getTargetLocation(index1));
        finish();
      }
    }
private:
  void addLeg(const GEOPOINT &w0, const GEOPOINT &w1,
              bool active, bool skip) {

    double NextLegDistance, NextLegBearing;
    double LegTime0;

    DistanceBearing(w0, w1, &NextLegDistance, &NextLegBearing);

    if (active) {
      calculated_info.LegDistanceToGo = NextLegDistance;
      if (in_aat_sector && (calculated_info.WaypointDistance<closedistance*3.0))
      {
        NextLegBearing = closebearing;
      }
    }

    double LegTime;
    double LegAltitude = GlidePolar::
      MacCreadyAltitude(maccready,
                        NextLegDistance, NextLegBearing,
                        calculated_info.WindSpeed,
                        calculated_info.WindBearing,
                        &(calculated_info.BestCruiseTrack),
                        &(calculated_info.VMacCready),
                        true,
                        &LegTime,
                        height_above_finish, cruise_efficiency);

    double LegAltitude0 = GlidePolar::
      MacCreadyAltitude(0,
                        NextLegDistance, NextLegBearing,
                        calculated_info.WindSpeed,
                        calculated_info.WindBearing,
                        0, 0,
                        true,
                        &LegTime0,
                        1.0e6, cruise_efficiency);

    if (LegTime0>=0.9*ERROR_TIME) {
      // can't make it, so assume flying at current mc
      LegAltitude0 = LegAltitude;
    }

    TaskAltitudeRequired += LegAltitude;
    TaskAltitudeRequired0 += LegAltitude0;

    calculated_info.TaskDistanceToGo += NextLegDistance;
    calculated_info.TaskTimeToGo += LegTime;

    if (calc_turning_now) {
      if (skip) {

        double NextLegDistanceTurningNow, NextLegBearingTurningNow;
        double LegTime_turningnow=0;

        DistanceBearing(gps_info.Location,
                        w1,
                        &NextLegDistanceTurningNow,
                        &NextLegBearingTurningNow);

        GlidePolar::
          MacCreadyAltitude(maccready,
                            NextLegDistanceTurningNow,
                            NextLegBearingTurningNow,
                            calculated_info.WindSpeed,
                            calculated_info.WindBearing,
                            0, 0,
                            true,
                            &LegTime_turningnow,
                            height_above_finish, cruise_efficiency);
        calculated_info.TaskTimeToGoTurningNow += LegTime_turningnow;
      } else {
        calculated_info.TaskTimeToGoTurningNow += LegTime;
      }
    }
    height_above_finish-= LegAltitude;
  };

private:
  bool in_aat_sector;
  bool calc_turning_now;
  double TaskAltitudeRequired;
  double TaskAltitudeRequired0;
  int FinalWayPoint;
  double height_above_finish;
  const double maccready;
  const double cruise_efficiency;
  unsigned activeIndex;
  double StartBestCruiseTrack;
  double finishHeight;
  const NMEA_INFO &gps_info;
  DERIVED_INFO &calculated_info;
  double closedistance;
  double closebearing;
};

void GlideComputerTask::TaskStatistics(const double this_maccready,
				       const double cruise_efficiency)
{

  double final_height = FAIFinishHeight(-1);

  TaskStatisticsVisitor tsv(Basic(), SetCalculated(),
                            this_maccready, cruise_efficiency,
                            final_height,
                            AATCloseDistance(),
                            task.ValidTaskPoint(task.getActiveIndex())
                            ? AATCloseBearing() : 0.0);
  task.scan_leg_reverse(tsv, false); // read lock
}

void GlideComputerTask::AATStats_Time() {
  // Task time to go calculations

  double aat_tasktime_elapsed = Basic().Time - Calculated().TaskStartTime;
  double aat_tasklength_seconds = task.getSettings().AATTaskLength*60;

  if (task.getActiveIndex()==0) {
    if (Calculated().AATTimeToGo==0) {
      SetCalculated().AATTimeToGo = aat_tasklength_seconds;
    }
  } else if (aat_tasktime_elapsed>=0) {
    SetCalculated().AATTimeToGo = max(0.0,
				  aat_tasklength_seconds
				  - aat_tasktime_elapsed);
  }

  if(task.Valid() && (Calculated().AATTimeToGo>0)) {
    SetCalculated().AATMaxSpeed =
      Calculated().AATMaxDistance / Calculated().AATTimeToGo;
    SetCalculated().AATMinSpeed =
      Calculated().AATMinDistance / Calculated().AATTimeToGo;
    SetCalculated().AATTargetSpeed =
      Calculated().AATTargetDistance / Calculated().AATTimeToGo;
  }
}

// TODO: turn into visitor
void GlideComputerTask::AATStats_Distance()
{
  int i;
  double MaxDistance, MinDistance, TargetDistance;

  MaxDistance = 0; MinDistance = 0; TargetDistance = 0;
  // Calculate Task Distances

  if(task.Valid()) {
    i=task.getActiveIndex();

    double LegToGo=0, TargetLegToGo=0;

    if (i > 0 ) { //RLD only include distance from glider to next leg
                  //if we've started the task
      LegToGo = Distance(Basic().Location,
                         task.getTaskPointLocation(i));

      TargetLegToGo = Distance(Basic().Location,
                               task.getTargetLocation(i));

      TASK_POINT tp = task.getTaskPoint(i);

      if(tp.AATType == AAT_CIRCLE) {
        MaxDistance = LegToGo + (tp.AATCircleRadius );  // ToDo: should be adjusted for angle of max target and for national rules
        MinDistance = LegToGo - (tp.AATCircleRadius );
      } else {
        MaxDistance = LegToGo + (tp.AATSectorRadius );  // ToDo: should be adjusted for angle of max target.
        MinDistance = LegToGo;
      }

      TargetDistance = TargetLegToGo;
    }

    i++;
    while(task.ValidTaskPoint(i)) {
      double LegDistance, TargetLegDistance;
      TASK_POINT tp = task.getTaskPoint(i);

      LegDistance = Distance(task.getTaskPointLocation(i),
                             task.getTaskPointLocation(i-1));

      TargetLegDistance = Distance(task.getTargetLocation(i),
                                   task.getTargetLocation(i-1));

      MaxDistance += LegDistance;
      MinDistance += LegDistance;

      if (tp.AATType == AAT_CIRCLE) {
        // breaking out single Areas increases accuracy for start
        // and finish

        // sector at start of (i)th leg
        if (i-1 == 0) {// first leg of task
          // add nothing
          MaxDistance -= task.getSettings().StartRadius; // e.g. Sports 2009 US Rules A116.3.2.  To Do: This should be configured multiple countries
          MinDistance -= task.getSettings().StartRadius;
        } else { // not first leg of task
          MaxDistance += (task.getTaskPoint(i-1).AATCircleRadius);  //ToDo: should be adjusted for angle of max target
          MinDistance -= (task.getTaskPoint(i-1).AATCircleRadius);  //ToDo: should be adjusted for angle of max target
        }

        // sector at end of ith leg
        if (!task.ValidTaskPoint(i+1)) {// last leg of task
          // add nothing
          MaxDistance -= task.getSettings().FinishRadius; // To Do: This can be configured for finish rules
          MinDistance -= task.getSettings().FinishRadius;
        } else { // not last leg of task
          MaxDistance += (tp.AATCircleRadius);  //ToDo: should be adjusted for angle of max target
          MinDistance -= (tp.AATCircleRadius);  //ToDo: should be adjusted for angle of max target
        }
      } else { // not circle (pie slice)
        // sector at start of (i)th leg
        if (i-1 == 0) {// first leg of task
          // add nothing
          MaxDistance += 0; // To Do: This can be configured for start rules
        } else { // not first leg of task
          MaxDistance += (task.getTaskPoint(i-1).AATCircleRadius);  //ToDo: should be adjusted for angle of max target
        }

        // sector at end of ith leg
        if (!task.ValidTaskPoint(i+1)) {// last leg of task
          // add nothing
          MaxDistance += 0; // To Do: This can be configured for finish rules
        } else { // not last leg of task
          MaxDistance += (tp.AATCircleRadius);  //ToDo: should be adjusted for angle of max target
        }
      }
      TargetDistance += TargetLegDistance;
      i++;
    }

    // JMW TODO accuracy: make these calculations more accurate, because
    // currently they are very approximate.

    SetCalculated().AATMaxDistance = MaxDistance;
    SetCalculated().AATMinDistance = MinDistance;
    SetCalculated().AATTargetDistance = TargetDistance;
  }
}

void GlideComputerTask::AATStats()
{
  if (!task.getSettings().AATEnabled
      || Calculated().ValidFinish) return ;

  AATStats_Distance();
  AATStats_Time();
}

void GlideComputerTask::CheckTransitionFinalGlide() {
  if (!task.Valid()) {
    SetCalculated().FinalGlide = 0;
    return;
  }

  const unsigned FinalWayPoint = task.getFinalWaypoint();
  // update final glide mode status
  if ((task.getActiveIndex() == FinalWayPoint)
       ||ForceFinalGlide) {
    if (Calculated().FinalGlide == 0)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE);
    SetCalculated().FinalGlide = 1;
  } else {
    if (Calculated().FinalGlide == 1)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
    SetCalculated().FinalGlide = 0;
  }
}

double GlideComputerTask::SpeedHeight() {
  if (Calculated().TaskDistanceToGo<=0) {
    return 0;
  }

  // Fraction of task distance covered
  double d_fraction = Calculated().TaskDistanceCovered/
    (Calculated().TaskDistanceCovered+Calculated().TaskDistanceToGo);

  double dh_start = Calculated().TaskStartAltitude;

  double dh_finish = FAIFinishHeight(-1);

  // Excess height
  return Calculated().NavAltitude
    - (dh_start*(1.0-d_fraction)+dh_finish*(d_fraction));
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


class TaskAltitudeRequiredVisitor:
  public AbsoluteTaskLegVisitor
{
public:
  TaskAltitudeRequiredVisitor(const double mc,
			      const double ce,
			      const double windspeed,
			      const double windbearing,
			      const double _min_start_height,
			      const double _min_finish_height):
    TotalAltitude(0.0),
    TotalDistance(0.0),
    TotalTime(0.0),
    Vfinal(0.0),
    maccready(mc),
    cruise_efficiency(ce),
    wind_speed(windspeed),
    wind_bearing(windbearing),
    reachable(false),
    min_start_height(_min_start_height),
    min_finish_height(_min_finish_height)
  {
    height_above_finish = min_start_height-min_finish_height;
  }
  void visit_single(TASK_POINT &point0, const unsigned index0)
  {
    visit_leg_intermediate(point0, index0, point0, index0);
  };
  void visit_leg_start(TASK_POINT &point0, const unsigned index0,
		       TASK_POINT &point1, const unsigned index1)
  {
    visit_leg_intermediate(point0, index0, point1, index1);
  };
  void visit_leg_intermediate(TASK_POINT &point0, const unsigned index0,
			      TASK_POINT &point1, const unsigned index1)
  {
    LegTime= 0.0;
    double LegAltitude =
      GlidePolar::MacCreadyAltitude(maccready,
                                    point1.LegDistance,
                                    point1.LegBearing,
                                    wind_speed,
                                    wind_bearing,
                                    0,
                                    0,
                                    true,
                                    &LegTime,
				    height_above_finish,
				    cruise_efficiency
                                    );
    // JMW CHECK FGAMT
    height_above_finish-= LegAltitude;

    TotalAltitude += LegAltitude;
    TotalDistance += point1.LegDistance;

    if (LegTime<0) {
      reachable = false;
    } else {
      TotalTime += LegTime;
    }
  };
  void visit_leg_final(TASK_POINT &point0, const unsigned index0,
		       TASK_POINT &point1, const unsigned index1)
  {
    visit_leg_intermediate(point0, index0, point1, index1);

    if (reachable) {
      Vfinal = point1.LegDistance/LegTime;
    } else {
      Vfinal = 0.0;
    }
  };
  double TotalAltitude;
  double TotalDistance;
  double TotalTime;
  double height_above_finish;
  double Vfinal;
  bool reachable;
private:
  double maccready;
  double cruise_efficiency;
  double wind_speed;
  double wind_bearing;
  double LegTime;
  double min_start_height;
  double min_finish_height;
};

bool GlideComputerTask::TaskAltitudeRequired(double this_maccready, double *Vfinal,
					     double *TotalTime, double *TotalDistance,
					     const double cruise_efficiency)
{
  // Calculate altitude required from start of task
  TaskAltitudeRequiredVisitor tarv(this_maccready, cruise_efficiency,
				   Calculated().WindSpeed,
				   Calculated().WindBearing,
				   FAIFinishHeight(0),
				   FAIFinishHeight(-1));
  task.scan_leg_reverse(tarv, false); // read lock

  SetCalculated().TaskAltitudeRequiredFromStart = tarv.TotalAltitude
    + FAIFinishHeight(-1);
  *TotalTime = tarv.TotalTime;
  *Vfinal = tarv.Vfinal;
  *TotalDistance = tarv.TotalDistance;

  return tarv.reachable;
}

double GlideComputerTask::MacCreadyOrAvClimbRate(double this_maccready)
{
  double mc_val = this_maccready;
  bool is_final_glide = false;

  if (Calculated().FinalGlide) {
    is_final_glide = true;
  }

  // when calculating 'achieved' task speed, need to use Mc if
  // not in final glide, or if in final glide mode and using
  // auto Mc, use the average climb rate achieved so far.

  if ((mc_val<0.1) ||
      (SettingsComputer().AutoMacCready &&
       ((SettingsComputer().AutoMcMode==0) ||
        ((SettingsComputer().AutoMcMode==2)&&(is_final_glide))
        ))
      ) {

    mc_val = Calculated().AdjustedAverageThermal;
  }
  return max(0.1, mc_val);

}

void GlideComputerTask::TaskSpeed(const double this_maccready,
				  const double cruise_efficiency)
{
  double TotalTime=0, TotalDistance=0, Vfinal=0;

  if (!task.Valid()) return;
  if (task.TaskIsTemporary()) return;
  if (Calculated().ValidFinish) return;
  if (!Calculated().Flying) return;

  // in case we leave early due to error
  SetCalculated().TaskSpeedAchieved = 0;
  SetCalculated().TaskSpeed = 0;

  if (task.getActiveIndex()<=0) { // no task speed before start
    SetCalculated().TaskSpeedInstantaneous = 0;
    return;
  }

  if (TaskAltitudeRequired(this_maccready, &Vfinal,
                           &TotalTime, &TotalDistance,
			   cruise_efficiency)) {

    double t0 = TotalTime;
    // total time expected for task

    double t1 = Basic().Time-Calculated().TaskStartTime;
    // time elapsed since start

    double d0 = TotalDistance;
    // total task distance

    double d1 = Calculated().TaskDistanceCovered;
    // actual distance covered

    double dr = Calculated().TaskDistanceToGo;
    // distance remaining

    double t2;
    // equivalent time elapsed after final glide

    double d2;
    // equivalent distance travelled after final glide

    double hf = FAIFinishHeight( -1);

    double h0 = Calculated().TaskAltitudeRequiredFromStart-hf;
    // total height required from start (takes safety arrival alt
    // and finish waypoint altitude into account)

    double h1 = max(0.0, Calculated().NavAltitude-hf);
    // height above target

    double dFinal;
    // final glide distance

    // equivalent speed
    double v2, v1;

    if ((t1<=0) || (d1<=0) || (d0<=0) || (t0<=0) || (h0<=0)) {
      // haven't started yet or not a real task
      SetCalculated().TaskSpeedInstantaneous = 0;
      //?      Calculated().TaskSpeed = 0;
      goto OnExit;
    }

    // JB's task speed...
    double hx = max(0.0, SpeedHeight());
    double t1mod = t1-hx/MacCreadyOrAvClimbRate(this_maccready);
    // only valid if flown for 5 minutes or more
    if (t1mod>300.0) {
      SetCalculated().TaskSpeedAchieved = d1/t1mod;
    } else {
      SetCalculated().TaskSpeedAchieved = d1/t1;
    }
    SetCalculated().TaskSpeed = Calculated().TaskSpeedAchieved;

    if (Vfinal<=0) {
      // can't reach target at current mc
      goto OnExit;
    }

    // distance that can be usefully final glided from here
    // (assumes average task glide angle of d0/h0)
    // JMW TODO accuracy: make this more accurate by working out final glide
    // through remaining turnpoints.  This will more correctly account
    // for wind.

    dFinal = min(dr, d0*min(1.0,max(0.0,h1/h0)));

    if (Calculated().ValidFinish) {
      dFinal = 0;
    }

    double dc = max(0.0, dr - dFinal);
    // amount of extra distance to travel in cruise/climb before final glide

    // equivalent distance to end of final glide
    d2 = d1+dFinal;

    // time at end of final glide
    t2 = t1+dFinal/Vfinal;

    // actual task speed achieved so far
    v1 = d1/t1;

#ifdef OLDTASKSPEED
    // average speed to end of final glide from here
    v2 = d2/t2;
    Calculated().TaskSpeed = max(v1,v2);
#else
    // average speed to end of final glide from here, weighted
    // according to how much extra time would be spent in cruise/climb
    // the closer dc (the difference between remaining distance and
    // final glidable distance) gets to zero, the closer v2 approaches
    // the average speed to end of final glide from here
    // in other words, the more we consider the final glide part to have
    // been earned.

    // this will be bogus at fast starts though...
    if (v1>0) {
      v2 = (d1+dc+dFinal)/(t1+dc/v1+dFinal/Vfinal);
    } else {
      v2 = (d1+dFinal)/(t1+dFinal/Vfinal);
    }
    SetCalculated().TaskSpeed = v2;
#endif

    double konst;
    if (logger.isTaskDeclared()) {
      konst = 1.0;
    } else {
      konst = 1.1;
    }

    double termikLigaPoints = 0;
    if (d1 > 0) {
      termikLigaPoints = konst*(0.015*0.001*d1-(400.0/(0.001*d1))+12.0)
        *v1*3.6*100.0/(double)SettingsComputer().Handicap;
    }

    SetCalculated().TermikLigaPoints = termikLigaPoints;

    if (time_advanced()) {
      double dt = Basic().Time-LastBasic().Time;
      // Calculate contribution to average task speed.
      // This is equal to the change in virtual distance
      // divided by the time step

      // This is a novel concept.
      // When climbing at the MC setting, this number should
      // be similar to the estimated task speed.
      // When climbing slowly or when flying off-course,
      // this number will drop.
      // In cruise at the optimum speed in zero lift, this
      // number will be similar to the estimated task speed.

      // A low pass filter is applied so it doesn't jump around
      // too much when circling.

      // If this number is higher than the overall task average speed,
      // it means that the task average speed is increasing.

      // When cruising in sink, this number will decrease.
      // When cruising in lift, this number will increase.

      // Therefore, it shows well whether at any time the glider
      // is wasting time.

      double mc_safe = max(0.1,this_maccready);
      double Vstar = max(1.0,Calculated().VMacCready);
      double vthis = (Calculated().LegDistanceCovered-
		      LastCalculated().LegDistanceCovered)/dt;
      vthis /= AirDensityRatio(Calculated().NavAltitude);

      double ttg = max(1.0, Calculated().LegTimeToGo);
      //      double Vav = d0/max(1.0,t0);
      double Vrem = Calculated().LegDistanceToGo/ttg;
      double Vref = // Vav;
	Vrem;
      double sr = -GlidePolar::SinkRate(Vstar);
      double height_diff = max(0.0, -Calculated().TaskAltitudeDifference);

      if (Calculated().timeCircling>30) {
	mc_safe = max(this_maccready,
		      Calculated().TotalHeightClimb/Calculated().timeCircling);
      }
      // circling percentage during cruise/climb
      double rho_cruise = max(0.0,min(1.0,mc_safe/(sr+mc_safe)));
      double rho_climb = 1.0-rho_cruise;
      double time_climb = height_diff/mc_safe;

      // calculate amount of time in cruise/climb glide
      double rho_c = max(0.0, min(1.0, time_climb / ttg));

      if (Calculated().FinalGlide) {
	if (rho_climb>0) {
	  rho_c = max(0.0, min(1.0, rho_c / rho_climb));
	}
	if (!Calculated().Circling) {
	  if (Calculated().TaskAltitudeDifference>0) {
	    rho_climb *= rho_c;
	    rho_cruise *= rho_c;
	    // Vref = Vrem;
	  }
	}
      }

      double w_comp = min(10.0,max(-10.0,Calculated().Vario/mc_safe));
      double vdiff = vthis/Vstar + w_comp*rho_cruise + rho_climb;

      if (vthis > SettingsComputer().SAFTEYSPEED*2) {
	vdiff = 1.0;
	// prevent funny numbers when starting mid-track
      }
      //      Calculated().Experimental = vdiff*100.0;

      vdiff *= Vref;

      if (t1<5) {
        SetCalculated().TaskSpeedInstantaneous = vdiff;
        // initialise
      } else {
	static double tsi_av = 0;
	static int n_av = 0;
        if ((task.getActiveIndex()
             ==LastCalculated().ActiveTaskPoint)
	    && (Calculated().LegDistanceToGo>1000.0)
	    && (Calculated().LegDistanceCovered>1000.0)) {

          SetCalculated().TaskSpeedInstantaneous =
            LowPassFilter(Calculated().TaskSpeedInstantaneous, vdiff, 0.1);

          // update stats
	  if (time_retreated()) {
	    tsi_av = 0;
	    n_av = 0;
          } else if (n_av>=60) {
	    tsi_av/= n_av;

	    SaveTaskSpeed(max((Basic().Time-Calculated().TaskStartTime)/3600.0,
			      max(0.0, min(100.0, tsi_av))));

	    tsi_av = 0;
	    n_av = 0;
          }
	  tsi_av += Calculated().TaskSpeedInstantaneous;
	  n_av ++;

        } else {

          SetCalculated().TaskSpeedInstantaneous =
            LowPassFilter(Calculated().TaskSpeedInstantaneous, vdiff, 0.5);

	  //	  Calculated().TaskSpeedInstantaneous = vdiff;
	  tsi_av = 0;
	  n_av = 0;
	}
      }
    }
  }
 OnExit:
  {};
}

void
GlideComputerTask::CheckFinalGlideThroughTerrain(double LegToGo, double LegBearing)
{
  // Final glide through terrain updates
  if (Calculated().FinalGlide) {

    GEOPOINT loc;
    bool out_of_range;
    double distance_soarable =
      FinalGlideThroughTerrain(LegBearing,
                               &Basic(), &Calculated(),
			       SettingsComputer(),
                               &loc,
                               LegToGo, &out_of_range, NULL);

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
}

void
GlideComputerTask::ResetEnter()
{
  aatdistance.ResetEnterTrigger(task.getActiveIndex());
}

void
GlideComputerTask::DoAutoMacCready(double mc_setting)
{
  bool is_final_glide = false;

  if (!SettingsComputer().AutoMacCready) return;

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
      mc_new = Calculated().AdjustedAverageThermal;
    }

  // if (on task, on final glide and activated at settings)
  } else if ( ((SettingsComputer().AutoMcMode==0)
	       ||(SettingsComputer().AutoMcMode==2)) && is_final_glide) {

    double time_remaining = Basic().Time-Calculated().TaskStartTime-9000;
    if (SettingsComputer().EnableOLC
	&& (SettingsComputer().OLCRules==0)
	&& (Calculated().NavAltitude>Calculated().TaskStartAltitude)
	&& (time_remaining>0)) {

      mc_new = MacCreadyTimeLimit(&Basic(), &Calculated(),
				  Calculated().WaypointBearing,
				  time_remaining,
				  Calculated().TaskStartAltitude);

    } else if (Calculated().TaskAltitudeDifference0>0) {

      // only change if above final glide with zero Mc
      // otherwise when we are well below, it will wind Mc back to
      // zero

      double slope =
	(Calculated().NavAltitude + Calculated().EnergyHeight
	 - FAIFinishHeight(task.getActiveIndex()))/
	(Calculated().WaypointDistance+1);

      double mc_pirker = PirkerAnalysis(&Basic(), &Calculated(),
					Calculated().WaypointBearing,
					slope);
      mc_pirker = max(0.0, mc_pirker);
      if (first_mc) {
	// don't allow Mc to wind down to zero when first achieving
	// final glide; but do allow it to wind down after that
	if (mc_pirker >= mc_new) {
	  mc_new = mc_pirker;
	  first_mc = false;
	} else if (SettingsComputer().AutoMcMode==2) {
	  // revert to averager based auto Mc
	  if (Calculated().AdjustedAverageThermal>0) {
	    mc_new = Calculated().AdjustedAverageThermal;
	  }
	}
      } else {
	mc_new = mc_pirker;
      }
    } else { // below final glide at zero Mc, never achieved final glide
      if (first_mc && (SettingsComputer().AutoMcMode==2)) {
	// revert to averager based auto Mc
	if (Calculated().AdjustedAverageThermal>0) {
	  mc_new = Calculated().AdjustedAverageThermal;
	}
      }
    }
  } else if ( (SettingsComputer().AutoMcMode==1)
	      || ((SettingsComputer().AutoMcMode==2)&& !is_final_glide) ) {
    if (Calculated().AdjustedAverageThermal>0) {
      mc_new = Calculated().AdjustedAverageThermal;
    }
  }

  GlidePolar::SetMacCready(LowPassFilter(mc_setting, mc_new, 0.15));
}

void
GlideComputerTask::SetLegStart()
{
  SetCalculated().LegStartTime = Basic().Time;
}

// JMW this is slow way to do things...
static bool CheckLandableReachableTerrain(const NMEA_INFO *Basic,
                                          const DERIVED_INFO *Calculated,
					  const SETTINGS_COMPUTER &settings,
                                          double LegToGo,
                                          double LegBearing) {
  bool out_of_range;
  double distance_soarable =
    FinalGlideThroughTerrain(LegBearing,
                             Basic, Calculated,
			     settings,
                             NULL,
                             LegToGo, &out_of_range, NULL);

  if ((out_of_range)||(distance_soarable> LegToGo)) {
    return true;
  } else {
    return false;
  }
}

class WaypointReachable: public WaypointVisitor {
public:
  WaypointReachable(const NMEA_INFO &_gps_info,
                    const DERIVED_INFO &_calculated_info,
                    const SETTINGS_COMPUTER &_settings):
    gps_info(_gps_info),
    calculated_info(_calculated_info),
    settings(_settings),
    narrow(true),
    reachable(false)
    {};

  void waypoint_landable(WAYPOINT &waypoint, WPCALC &wpcalc, const unsigned i)
    {
      // treat landables as airports
      waypoint_airport(waypoint, wpcalc, i);
    }
  void waypoint_airport(WAYPOINT &waypoint, WPCALC &wpcalc, const unsigned i)
    {
      if (narrow) {
        if (!wpcalc.Visible && !wpcalc.InTask) {
          return;
        }
      } else {
        if (wpcalc.Visible || !wpcalc.FarVisible) {
          return;
        }
      }
      double WaypointDistance, WaypointBearing,
        AltitudeRequired,AltitudeDifference;

      DistanceBearing(gps_info.Location,
		      waypoint.Location,
		      &WaypointDistance,
		      &WaypointBearing);

      if (!narrow && (WaypointDistance>100000.0) && !wpcalc.InTask) {
        // already processed if in task, or too far away to calculate
        return;
      }

      AltitudeRequired =
	GlidePolar::MacCreadyAltitude
	(GlidePolar::SafetyMacCready,
	 WaypointDistance,
	 WaypointBearing,
	 calculated_info.WindSpeed,
	 calculated_info.WindBearing,
	 0,0,true,0);
      AltitudeRequired = AltitudeRequired + settings.SAFETYALTITUDEARRIVAL
	+ waypoint.Altitude ;
      AltitudeDifference = calculated_info.NavAltitude - AltitudeRequired;
      wpcalc.AltArrivalAGL = AltitudeDifference;

      if(AltitudeDifference <0){
	wpcalc.Reachable = false;
      } else {
	wpcalc.Reachable = true;
	if (!reachable || wpcalc.InTask) {
	  if (CheckLandableReachableTerrain(&gps_info,
					    &calculated_info,
					    settings,
					    WaypointDistance,
					    WaypointBearing)) {
	    reachable = true;
	  } else if (wpcalc.InTask) {
            // non-task waypoint reachability is not calculated with
            // respect to glide through terrain (because it is too slow)
	    wpcalc.Reachable = false;
	  }
	}
      }
    }
  bool narrow;
  bool reachable;
private:
  const NMEA_INFO &gps_info;
  const DERIVED_INFO &calculated_info;
  const SETTINGS_COMPUTER &settings;
};

/**
 * Calculates whether there is a reachable landable waypoint
 */
void
GlideComputerTask::CalculateWaypointReachable(void)
{
  WaypointReachable wrv(Basic(), Calculated(), SettingsComputer());
  WaypointScan::scan_forward(wrv);

  if (!wrv.reachable) {
    wrv.narrow = false;
    WaypointScan::scan_forward(wrv);
    // widen search to far visible waypoints
    // (only do this if can't see one at present)
  }
  SetCalculated().LandableReachable = wrv.reachable;
}

