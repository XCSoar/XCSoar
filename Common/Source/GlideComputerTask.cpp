/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "Settings.hpp"
#include "SettingsTask.hpp"
#include "WayPoint.hpp"
#include "Math/Earth.hpp"
#include "Math/Geometry.hpp"
#include "Math/Pressure.h"
#include "Math/LowPassFilter.hpp"
#include "McReady.h"
#include "GlideRatio.hpp"
#include "GlideSolvers.hpp"
#include "Abort.hpp"
#include "Logger.h"
#include "InputEvents.h"
// JMW TODO: abstract up to higher layer so a base copy of this won't 
// call any event

#define AUTOADVANCE_MANUAL 0
#define AUTOADVANCE_AUTO 1
#define AUTOADVANCE_ARM 2
#define AUTOADVANCE_ARMSTART 3


void GlideComputerTask::ResetFlight(const bool full)
{
  if (full) {
    olc.ResetFlight();
    aatdistance.Reset();
  }
}


void GlideComputerTask::StartTask(const bool do_advance,
				  const bool do_announce) 
{
  ActiveWayPoint=1;
  SelectedWaypoint = ActiveWayPoint;
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
    TaskStatistics(mc, ce);
    AATStats();
    TaskSpeed(mc, ce);
  }
}

void
GlideComputerTask::ProcessIdle()
{
  if (isTaskAborted()) { 
    SortLandableWaypoints();
  }
  DoBestAlternateSlow();
}

bool GlideComputerTask::DoLogging() {
  if (Calculated().Flying && olc_clock.check_advance(Basic().Time)) {
    bool restart = olc.addPoint(Basic().Longitude,
				Basic().Latitude,
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

  if (!ValidWayPoint(HomeWaypoint)) {
    SetCalculated().HomeDistance = 0.0;
    SetCalculated().HomeRadial = 0.0; // VENTA3
    return;
  }

  double w1lat = WayPointList[HomeWaypoint].Latitude;
  double w1lon = WayPointList[HomeWaypoint].Longitude;
  double w0lat = Basic().Latitude;
  double w0lon = Basic().Longitude;

  DistanceBearing(w1lat, w1lon,
                  w0lat, w0lon,
                  &SetCalculated().HomeDistance, &SetCalculated().HomeRadial);

}


void GlideComputerTask::DistanceToNext()
{
  ScopeLock protect(mutexTaskData);

  if(ValidTaskPoint(ActiveWayPoint))
    {
      double w1lat, w1lon;
      double w0lat, w0lon;

      w0lat = WayPointList[task_points[ActiveWayPoint].Index].Latitude;
      w0lon = WayPointList[task_points[ActiveWayPoint].Index].Longitude;
      DistanceBearing(Basic().Latitude, Basic().Longitude,
                      w0lat, w0lon,
                      &SetCalculated().WaypointDistance,
                      &SetCalculated().WaypointBearing);

      SetCalculated().ZoomDistance = Calculated().WaypointDistance;

      if (AATEnabled && !TaskIsTemporary()
	  && (ActiveWayPoint>0) &&
          ValidTaskPoint(ActiveWayPoint+1)) {

        w1lat = task_points[ActiveWayPoint].AATTargetLat;
        w1lon = task_points[ActiveWayPoint].AATTargetLon;

        DistanceBearing(Basic().Latitude, Basic().Longitude,
                        w1lat, w1lon,
                        &SetCalculated().WaypointDistance,
                        &SetCalculated().WaypointBearing);

        if (Calculated().WaypointDistance>AATCloseDistance()*3.0) {
          SetCalculated().ZoomDistance = max(Calculated().WaypointDistance,
                                         Calculated().ZoomDistance);
        } else {
	  SetCalculated().WaypointBearing = AATCloseBearing();
        }

      } else if ((ActiveWayPoint==0) && (ValidTaskPoint(ActiveWayPoint+1))
                 && (Calculated().IsInSector) &&
		 !TaskIsTemporary()) {

        // JMW set waypoint bearing to start direction if in start sector

        if (AATEnabled) {
          w1lat = task_points[ActiveWayPoint+1].AATTargetLat;
          w1lon = task_points[ActiveWayPoint+1].AATTargetLon;
        } else {
          w1lat = WayPointList[task_points[ActiveWayPoint+1].Index].Latitude;
          w1lon = WayPointList[task_points[ActiveWayPoint+1].Index].Longitude;
        }

        DistanceBearing(Basic().Latitude, Basic().Longitude,
                        w1lat, w1lon,
                        NULL,
                        &SetCalculated().WaypointBearing);
      }
    }
  else
    {
      SetCalculated().ZoomDistance = 0;
      SetCalculated().WaypointDistance = 0;
      SetCalculated().WaypointBearing = 0;
    }
}


void GlideComputerTask::AltitudeRequired(const double this_maccready,
					 const double cruise_efficiency)
{
  ScopeLock protect(mutexTaskData);
  if(ValidTaskPoint(ActiveWayPoint))
    {
      double wp_alt = FAIFinishHeight(ActiveWayPoint);
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
    }
  else
    {
      SetCalculated().NextAltitudeRequired = 0;
      SetCalculated().NextAltitudeDifference = 0;
      SetCalculated().NextAltitudeDifference0 = 0; // VENTA6
    }
}


double GlideComputerTask::AATCloseBearing() const
{
  // ensure waypoint goes in direction of track if very close
  double course_bearing;
  DistanceBearing(task_points[ActiveWayPoint-1].AATTargetLat,
		  task_points[ActiveWayPoint-1].AATTargetLon,
		  Basic().Latitude,
		  Basic().Longitude,
		  NULL, &course_bearing);

  course_bearing = AngleLimit360(course_bearing+
				 task_points[ActiveWayPoint].AATTargetOffsetRadial);
  return course_bearing;
}


double
FAIFinishHeight(const SETTINGS_COMPUTER &settings,
		const DERIVED_INFO& Calculated, int wp)
{
  int FinalWayPoint = getFinalWaypoint();
  if (wp== -1) {
    wp = FinalWayPoint;
  }
  double wp_alt;
  if(ValidTaskPoint(wp)) {
    wp_alt = WayPointList[task_points[wp].Index].Altitude;
  } else {
    wp_alt = 0;
  }

  if (!TaskIsTemporary() && (wp==FinalWayPoint)) {
    if (EnableFAIFinishHeight && !AATEnabled) {
      return max(max(FinishMinHeight, 
		     settings.SAFETYALTITUDEARRIVAL)+ wp_alt,
                 Calculated.TaskStartAltitude-1000.0);
    } else {
      return max(FinishMinHeight, 
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

  if (!ValidTaskPoint(the_turnpoint)) return false;

  if(SectorType==0)
    {
      if(Calculated().WaypointDistance < SectorRadius)
        {
          return true;
        }
    }
  if (SectorType>0)
    {
      mutexTaskData.Lock();
      DistanceBearing(WayPointList[task_points[the_turnpoint].Index].Latitude,
                      WayPointList[task_points[the_turnpoint].Index].Longitude,
                      Basic().Latitude ,
                      Basic().Longitude,
                      NULL, &AircraftBearing);
      mutexTaskData.Unlock();

      AircraftBearing = AircraftBearing - task_points[the_turnpoint].Bisector ;
      while (AircraftBearing<-180) {
        AircraftBearing+= 360;
      }
      while (AircraftBearing>180) {
        AircraftBearing-= 360;
      }

      if (SectorType==2) {
        // JMW added german rules
        if (Calculated().WaypointDistance<500) {
          return true;
        }
      }
      if( (AircraftBearing >= -45) && (AircraftBearing <= 45))
        {
          if (SectorType==1) {
            if(Calculated().WaypointDistance < SectorRadius)
              {
                return true;
              }
          } else {
            // JMW added german rules
            if(Calculated().WaypointDistance < 10000)
              {
                return true;
              }
          }
        }
    }
  return false;
}


bool GlideComputerTask::ValidFinish( ) const
{
  if ((FinishMinHeight>0)
      &&(Calculated().TerrainValid)
      &&(Calculated().AltitudeAGL<FinishMinHeight)) {
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

  if (!WayPointList) return false;

  if (!ValidFinish()) return false;

  // Finish invalid
  if (!ValidTaskPoint(i)) return false;

  ScopeLock protect(mutexTaskData);

  // distance from aircraft to start point
  DistanceBearing(Basic().Latitude,
                  Basic().Longitude,
                  WayPointList[task_points[i].Index].Latitude,
                  WayPointList[task_points[i].Index].Longitude,
                  &FirstPointDistance,
                  &AircraftBearing);

  bool InFinishSector = LastCalculated().InFinishSector;

  bool inrange = false;
  inrange = (FirstPointDistance<FinishRadius);
  if (!inrange) {
    InFinishSector = false;
  }

  if(!FinishLine) // Start Circle
    {
      retval = inrange;
      goto OnExit;
    }

  // Finish line
  AircraftBearing = AngleLimit180(AircraftBearing - task_points[i].InBound);

  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  if(FinishLine==1) { // Finish line
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
  if (StartMaxSpeed!=0) {
    if (Basic().AirspeedAvailable) {
      if (Basic().IndicatedAirspeed>(StartMaxSpeed+Margin))
        valid = false;
    } else {
      if (Basic().Speed>(StartMaxSpeed+Margin))
        valid = false;
    }
  }
  return valid;
}


bool GlideComputerTask::InsideStartHeight(const DWORD Margin) const
{
  bool valid = true;
  if ((StartMaxHeight!=0)&&(Calculated().TerrainValid)) {
    if (StartHeightRef == 0) {
      if (Calculated().AltitudeAGL>(StartMaxHeight+Margin))
	valid = false;
    } else {
      if (Calculated().NavAltitude>(StartMaxHeight+Margin))
	valid = false;
    }
  }
  return valid;
}

bool GlideComputerTask::InStartSector_Internal(int Index,
					       double OutBound,
					       bool &LastInSector)
{
  if (!ValidWayPoint(Index)) return false;

  // No Task Loaded

  double AircraftBearing;
  double FirstPointDistance;

  // distance from aircraft to start point
  DistanceBearing(Basic().Latitude,
                  Basic().Longitude,
                  WayPointList[Index].Latitude,
                  WayPointList[Index].Longitude,
                  &FirstPointDistance,
                  &AircraftBearing);

  bool inrange = false;
  inrange = (FirstPointDistance<StartRadius);

  if(StartLine==0) {
    // Start Circle
    return inrange;
  }

  // Start Line
  AircraftBearing = AngleLimit180(AircraftBearing - OutBound);

  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  if(StartLine==1) { // Start line
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
      !ValidTaskPoint(ActiveWayPoint) ||
      !ValidTaskPoint(0))
    return false;

  ScopeLock protect(mutexTaskData);

  bool in_height = true;

  if ((ActiveWayPoint>0)
      && !ValidTaskPoint(ActiveWayPoint+1)) {
    // don't detect start if finish is selected
    retval = false;
    goto OnExit;
  }

  in_height = InsideStartHeight(StartMaxHeightMargin);

  if ((task_points[0].Index != Calculated().StartSectorWaypoint) 
      && (Calculated().StartSectorWaypoint>=0)) {
    LastInStartSector = false;
    SetCalculated().StartSectorWaypoint = task_points[0].Index;
  }

  isInSector = in_height & InStartSector_Internal(task_points[0].Index, 
						  task_points[0].OutBound,
						  LastInStartSector);

  *CrossedStart = LastInStartSector && !isInSector;
  LastInStartSector = isInSector;
  if (*CrossedStart) {
    goto OnExit;
  }

  if (EnableMultipleStartPoints) {
    for (int i=0; i<MAXSTARTPOINTS; i++) {
      if (task_start_points[i].Active && (task_start_points[i].Index>=0)
          && (task_start_points[i].Index != task_points[0].Index)) {

        retval = in_height & InStartSector_Internal(task_start_points[i].Index,
						    task_start_points[i].OutBound,
						    task_start_points[i].InSector);
        isInSector |= retval;

        int index = task_start_points[i].Index;
        *CrossedStart = task_start_points[i].InSector && !retval;
        task_start_points[i].InSector = retval;
        if (*CrossedStart) {
          if (task_points[0].Index != index) {
            task_points[0].Index = index;
            LastInStartSector = false;
            SetCalculated().StartSectorWaypoint = index;
            RefreshTask();
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
  if (AutoAdvance== AUTOADVANCE_AUTO) {
    return true;
  }
  if ((AutoAdvance== AUTOADVANCE_ARM) || (AutoAdvance==AUTOADVANCE_ARMSTART)) {
    if (AdvanceArmed) {
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

  SetCalculated().ActiveWayPoint = ActiveWayPoint;

  if (!Calculated().Flying) {
    SetCalculated().ReadyWayPoint = -1;
    return false;
  }

  if (AutoAdvance== AUTOADVANCE_AUTO) {
    if (reset) {
      AdvanceArmed = false;
    }
    return true;
  }
  if (AutoAdvance== AUTOADVANCE_ARM) {
    if (AdvanceArmed) {
      if (reset) AdvanceArmed = false;
      return true;
    } else {
      say_ready = true;
    }
  }
  if (AutoAdvance== AUTOADVANCE_ARMSTART) {
    if ((Calculated().ActiveWayPoint == 0) || restart) {
      if (!AdvanceArmed) {
        say_ready = true;
      } else if (reset) {
        AdvanceArmed = false;
        return true;
      }
    } else {
      // JMW fixed 20070528
      if (Calculated().ActiveWayPoint>0) {
        if (reset) AdvanceArmed = false;
        return true;
      }
    }
  }

  // see if we've gone back a waypoint (e.g. restart)
  if (Calculated().ActiveWayPoint < LastCalculated().ActiveWayPoint) {
    SetCalculated().ReadyWayPoint = -1;
  }

  if (say_ready) {
    if (Calculated().ActiveWayPoint != LastCalculated().ReadyWayPoint) {
      InputEvents::processGlideComputer(GCE_ARM_READY);
      SetCalculated().ReadyWayPoint = Calculated().ActiveWayPoint;
    }
  }
  return false;
}


void GlideComputerTask::CheckStart() {
  bool StartCrossed= false;

  if (InStartSector(&StartCrossed)) {
    SetCalculated().IsInSector = true;

    if (ReadyToStart()) {
      aatdistance.AddPoint(Basic().Longitude,
			   Basic().Latitude,
			   0,
			   AATCloseDistance());
    }
    // TODO: we are ready to start even when outside start rules but
    // within margin
    if (ValidStartSpeed(StartMaxSpeedMargin)) {
      ReadyToAdvance(false, true);
    }
    // TODO accuracy: monitor start speed throughout time in start sector
  }
  if (StartCrossed) {
    // TODO: Check whether speed and height are within the rules or
    // not (zero margin)
    if(!IsFinalWaypoint() && ValidStartSpeed() && InsideStartHeight()) {

      // This is set whether ready to advance or not, because it will
      // appear in the flight log, so if it's valid, it's valid.
      SetCalculated().ValidStart = true;

      if (ReadyToAdvance(true, true)) {
        ActiveWayPoint=0; // enforce this since it may be 1
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

      if ((ActiveWayPoint<=1)
          && !IsFinalWaypoint()
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
            ActiveWayPoint=0; // enforce this since it may be 1
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
     &&(ActiveWayPoint<=1)) {
    CheckStart();
  }
}


void GlideComputerTask::CheckFinish() {
  if (InFinishSector(ActiveWayPoint)) {
    SetCalculated().IsInSector = true;
    aatdistance.AddPoint(Basic().Longitude,
                         Basic().Latitude,
                         ActiveWayPoint,
			 AATCloseDistance());
    if (!Calculated().ValidFinish) {
      SetCalculated().ValidFinish = true;
      AnnounceWayPointSwitch(false);
      SaveFinish();
    }
  }
}


void GlideComputerTask::AddAATPoint(int taskwaypoint) {
  bool insector = false;
  if (taskwaypoint>0) {
    if (AATEnabled) {
      insector = InAATTurnSector(Basic().Longitude,
                                 Basic().Latitude, taskwaypoint);
    } else {
      insector = InTurnSector(taskwaypoint);
    }
    if(insector) {
      if (taskwaypoint == ActiveWayPoint) {
        SetCalculated().IsInSector = true;
      }
      aatdistance.AddPoint(Basic().Longitude,
                           Basic().Latitude,
                           taskwaypoint,
			   AATCloseDistance());
    }
  }
}


void GlideComputerTask::CheckInSector() {

  if (ActiveWayPoint>0) {
    AddAATPoint(ActiveWayPoint-1);
  }
  AddAATPoint(ActiveWayPoint);

  // JMW Start bug XXX

  if (aatdistance.HasEntered(ActiveWayPoint)) {
    if (ReadyToAdvance(true, false)) {
      AnnounceWayPointSwitch(true);
    }
    if (Calculated().Flying) {
      SetCalculated().ValidFinish = false;
    }
  }
}


void GlideComputerTask::InSector()
{
  if (ActiveWayPoint<0) return;

  SetCalculated().IsInSector = false;

  if(ActiveWayPoint == 0) {
    CheckStart();
  } else {
    if(IsFinalWaypoint()) {
      AddAATPoint(ActiveWayPoint-1);
      CheckFinish();
    } else {
      CheckRestart();
      if (ActiveWayPoint>0) {
        CheckInSector();
      }
    }
  }
}



////

void GlideComputerTask::LDNext(const double LegToGo) {
  double height_above_leg = Calculated().NavAltitude+Calculated().EnergyHeight
    - FAIFinishHeight(ActiveWayPoint);

  SetCalculated().LDNext = UpdateLD(Calculated().LDNext,
				    LegToGo,
				    height_above_leg,
				    0.5);
}


void GlideComputerTask::CheckForceFinalGlide() {
  // Auto Force Final Glide forces final glide mode
  // if above final glide...
  if (isTaskAborted()) {
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


void GlideComputerTask::TaskStatistics(const double this_maccready,
				       const double cruise_efficiency)
{

  if (!ValidTaskPoint(ActiveWayPoint) ||
      ((ActiveWayPoint>0) && !ValidTaskPoint(ActiveWayPoint-1))) {

    SetCalculated().LegSpeed = 0;
    SetCalculated().LegDistanceToGo = 0;
    SetCalculated().LegDistanceCovered = 0;
    SetCalculated().LegTimeToGo = 0;

    if (!AATEnabled) {
      SetCalculated().AATTimeToGo = 0;
    }

    //    Calculated().TaskSpeed = 0;

    SetCalculated().TaskDistanceToGo = 0;
    SetCalculated().TaskDistanceCovered = 0;
    SetCalculated().TaskTimeToGo = 0;
    SetCalculated().TaskTimeToGoTurningNow = -1;

    SetCalculated().TaskAltitudeRequired = 0;
    SetCalculated().TaskAltitudeDifference = 0;
    SetCalculated().TaskAltitudeDifference0 = 0;

    SetCalculated().TerrainWarningLatitude = 0.0;
    SetCalculated().TerrainWarningLongitude = 0.0;

    SetCalculated().LDFinish = INVALID_GR;
    SetCalculated().GRFinish = INVALID_GR; // VENTA-ADDON
    SetCalculated().LDNext = INVALID_GR;

    SetCalculated().FinalGlide = 0;
    CheckFinalGlideThroughTerrain(0.0, 0.0);

    // no task selected, so work things out at current heading

    GlidePolar::MacCreadyAltitude(this_maccready, 100.0,
                                  Basic().TrackBearing,
                                  Calculated().WindSpeed,
                                  Calculated().WindBearing,
                                  &(SetCalculated().BestCruiseTrack),
                                  &(SetCalculated().VMacCready),
                                  (Calculated().FinalGlide==1),
                                  NULL, 1.0e6, cruise_efficiency);

    return;
  }

  ScopeLock protect(mutexTaskData);

  ///////////////////////////////////////////////
  // Calculate Task Distances
  // First calculate distances for this waypoint

  double LegCovered, LegToGo=0;
  double LegDistance, LegBearing=0;
  bool calc_turning_now;

  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;

  if (AATEnabled && (ActiveWayPoint>0) &&
      !TaskIsTemporary() && (ValidTaskPoint(ActiveWayPoint+1))) {
    w1lat = task_points[ActiveWayPoint].AATTargetLat;
    w1lon = task_points[ActiveWayPoint].AATTargetLon;
  } else {
    w1lat = WayPointList[task_points[ActiveWayPoint].Index].Latitude;
    w1lon = WayPointList[task_points[ActiveWayPoint].Index].Longitude;
  }

  DistanceBearing(Basic().Latitude,
                  Basic().Longitude,
                  w1lat,
                  w1lon,
                  &LegToGo, &LegBearing);

  if (AATEnabled && (ActiveWayPoint>0) && ValidTaskPoint(ActiveWayPoint+1)
      && Calculated().IsInSector && (this_maccready>0.1) &&
      !TaskIsTemporary()) {
    calc_turning_now = true;
  } else {
    calc_turning_now = false;
  }

  if ((ActiveWayPoint<1) || TaskIsTemporary()) {
    LegCovered = 0;
    if (!TaskIsTemporary()) { // RLD if task not started, exclude distance to start point
      LegToGo=0;
    }
   } else {
    if (AATEnabled) {
      // TODO accuracy: Get best range point to here...
      w0lat = task_points[ActiveWayPoint-1].AATTargetLat;
      w0lon = task_points[ActiveWayPoint-1].AATTargetLon;
    } else {
      w0lat = WayPointList[task_points[ActiveWayPoint-1].Index].Latitude;
      w0lon = WayPointList[task_points[ActiveWayPoint-1].Index].Longitude;
    }

    DistanceBearing(w1lat,
                    w1lon,
                    w0lat,
                    w0lon,
                    &LegDistance, NULL);

    LegCovered = ProjectedDistance(w0lon, w0lat,
                                   w1lon, w1lat,
                                   Basic().Longitude,
                                   Basic().Latitude);

    if ((StartLine==0) && (ActiveWayPoint==1)) {
      // Correct speed calculations for radius
      // JMW TODO accuracy: legcovered replace this with more accurate version
      // LegDistance -= StartRadius;
      LegCovered = max(0,LegCovered-StartRadius);
    }
  }

  SetCalculated().LegDistanceToGo = LegToGo;
  SetCalculated().LegDistanceCovered = LegCovered;
  SetCalculated().TaskDistanceCovered = LegCovered;

  if (Basic().Time > Calculated().LegStartTime) {
    SetLegStart();
    SetCalculated().LegSpeed = Calculated().LegDistanceCovered
      / (Basic().Time - Calculated().LegStartTime);
  }

  ///////////////////////////////////////////////////
  // Now add distances for start to previous waypoint

  if (!TaskIsTemporary()) {

    if (!AATEnabled) {
      for(int i=0;i< ActiveWayPoint-1; i++)
        {
          if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

          w1lat = WayPointList[task_points[i].Index].Latitude;
          w1lon = WayPointList[task_points[i].Index].Longitude;
          w0lat = WayPointList[task_points[i+1].Index].Latitude;
          w0lon = WayPointList[task_points[i+1].Index].Longitude;

          DistanceBearing(w1lat,
                          w1lon,
                          w0lat,
                          w0lon,
                          &LegDistance, NULL);
          SetCalculated().TaskDistanceCovered += LegDistance;
        }
    } else if (ActiveWayPoint>0) {
      // JMW added correction for distance covered
      SetCalculated().TaskDistanceCovered =
        aatdistance.DistanceCovered(Basic().Longitude,
                                    Basic().Latitude,
                                    ActiveWayPoint,
				    AATCloseDistance());
    }
  }

  ///////////////////////////////////////////////////////////

  CheckTransitionFinalGlide();

  // accumulators
  double TaskAltitudeRequired = 0;
  double TaskAltitudeRequired0 = 0;
  SetCalculated().TaskDistanceToGo = 0;
  SetCalculated().TaskTimeToGo = 0;
  SetCalculated().TaskTimeToGoTurningNow = 0;

  double LegTime0;

  // Calculate Final Glide To Finish

  int FinalWayPoint = getFinalWaypoint();

  double height_above_finish = Calculated().NavAltitude+
    Calculated().EnergyHeight-FAIFinishHeight(-1);

  //////////////////
  // Now add it for remaining waypoints
  int task_index= FinalWayPoint;

  double StartBestCruiseTrack = 0;

  if (!TaskIsTemporary()) {
    while ((task_index>ActiveWayPoint) && (ValidTaskPoint(task_index))) {
      double this_LegTimeToGo;
      bool this_is_final = (task_index==FinalWayPoint)
	|| ForceFinalGlide;

      this_is_final = true; // JMW CHECK FGAMT

      if (AATEnabled) {
	w1lat = task_points[task_index].AATTargetLat;
	w1lon = task_points[task_index].AATTargetLon;
	w0lat = task_points[task_index-1].AATTargetLat;
	w0lon = task_points[task_index-1].AATTargetLon;
      } else {
	w1lat = WayPointList[task_points[task_index].Index].Latitude;
	w1lon = WayPointList[task_points[task_index].Index].Longitude;
	w0lat = WayPointList[task_points[task_index-1].Index].Latitude;
	w0lon = WayPointList[task_points[task_index-1].Index].Longitude;
      }

      double NextLegDistance, NextLegBearing;

      DistanceBearing(w0lat,
		      w0lon,
		      w1lat,
		      w1lon,
		      &NextLegDistance, &NextLegBearing);

      double LegAltitude = GlidePolar::
	MacCreadyAltitude(this_maccready,
			  NextLegDistance, NextLegBearing,
			  Calculated().WindSpeed,
			  Calculated().WindBearing,
			  0, 0,
			  this_is_final,
			  &this_LegTimeToGo,
			  height_above_finish, cruise_efficiency);

      double LegAltitude0 = GlidePolar::
	MacCreadyAltitude(0,
			  NextLegDistance, NextLegBearing,
			  Calculated().WindSpeed,
			  Calculated().WindBearing,
			  0, 0,
			  true,
			  &LegTime0, 1.0e6, cruise_efficiency
			  );

      if (LegTime0>=0.9*ERROR_TIME) {
	// can't make it, so assume flying at current mc
	LegAltitude0 = LegAltitude;
      }

      TaskAltitudeRequired += LegAltitude;
      TaskAltitudeRequired0 += LegAltitude0;

      SetCalculated().TaskDistanceToGo += NextLegDistance;
      SetCalculated().TaskTimeToGo += this_LegTimeToGo;

      if (task_index==1) {
	StartBestCruiseTrack = NextLegBearing;
      }

      if (calc_turning_now) {
	if (task_index == ActiveWayPoint+1) {

	  double NextLegDistanceTurningNow, NextLegBearingTurningNow;
	  double this_LegTimeToGo_turningnow=0;

	  DistanceBearing(Basic().Latitude,
			  Basic().Longitude,
			  w1lat,
			  w1lon,
			  &NextLegDistanceTurningNow,
			  &NextLegBearingTurningNow);

	  GlidePolar::
	    MacCreadyAltitude(this_maccready,
			      NextLegDistanceTurningNow,
			      NextLegBearingTurningNow,
			      Calculated().WindSpeed,
			      Calculated().WindBearing,
			      0, 0,
			      this_is_final,
			      &this_LegTimeToGo_turningnow,
			      height_above_finish, cruise_efficiency);
	  SetCalculated().TaskTimeToGoTurningNow += this_LegTimeToGo_turningnow;
	} else {
	  SetCalculated().TaskTimeToGoTurningNow += this_LegTimeToGo;
	}
      }

      height_above_finish-= LegAltitude;

      task_index--;
    }
  }
  ////////////////


  /////// current waypoint, do this last!

  if (AATEnabled && !TaskIsTemporary()
      && (ActiveWayPoint>0) &&
      ValidTaskPoint(ActiveWayPoint+1) && Calculated().IsInSector) {
    if (Calculated().WaypointDistance<AATCloseDistance()*3.0) {
      LegBearing = AATCloseBearing();
    }
  }

  // JMW TODO accuracy: use mc based on risk? no!
  double LegAltitude =
    GlidePolar::MacCreadyAltitude(this_maccready,
                                  LegToGo,
                                  LegBearing,
                                  Calculated().WindSpeed,
                                  Calculated().WindBearing,
                                  &(SetCalculated().BestCruiseTrack),
                                  &(SetCalculated().VMacCready),

				  // (Calculated().FinalGlide==1),
				  true,  // JMW CHECK FGAMT

                                  &(SetCalculated().LegTimeToGo),
                                  height_above_finish, cruise_efficiency);

  double LegAltitude0 =
    GlidePolar::MacCreadyAltitude(0,
                                  LegToGo,
                                  LegBearing,
                                  Calculated().WindSpeed,
                                  Calculated().WindBearing,
                                  0,
                                  0,
                                  true,
                                  &LegTime0, 1.0e6, cruise_efficiency
                                  );

  if (Calculated().IsInSector && (ActiveWayPoint==0) && !TaskIsTemporary()) {
    // set best cruise track to first leg bearing when in start sector
    SetCalculated().BestCruiseTrack = StartBestCruiseTrack;
  }

  // JMW TODO accuracy: Use safetymc where appropriate

  LDNext(LegToGo);

  if (LegTime0>= 0.9*ERROR_TIME) {
    // can't make it, so assume flying at current mc
    LegAltitude0 = LegAltitude;
  }

  TaskAltitudeRequired += LegAltitude;
  TaskAltitudeRequired0 += LegAltitude0;
  SetCalculated().TaskDistanceToGo += LegToGo;
  SetCalculated().TaskTimeToGo += Calculated().LegTimeToGo;

  height_above_finish-= LegAltitude;

  ////////////////

  if (calc_turning_now) {
    SetCalculated().TaskTimeToGoTurningNow +=
      Basic().Time-Calculated().TaskStartTime;
  } else {
    SetCalculated().TaskTimeToGoTurningNow = -1;
  }

  double final_height = FAIFinishHeight(-1);

  double total_energy_height = Calculated().NavAltitude
    + Calculated().EnergyHeight;

  SetCalculated().TaskAltitudeRequired = TaskAltitudeRequired + final_height;

  TaskAltitudeRequired0 += final_height;

  SetCalculated().TaskAltitudeDifference = total_energy_height
    - Calculated().TaskAltitudeRequired;

  SetCalculated().TaskAltitudeDifference0 = total_energy_height
    - TaskAltitudeRequired0;

  // VENTA6
  SetCalculated().NextAltitudeDifference0 = total_energy_height
    - Calculated().NextAltitudeRequired0;

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

  CheckFinalGlideThroughTerrain(LegToGo, LegBearing);

  CheckForceFinalGlide();
}


void GlideComputerTask::AATStats_Time() {
  // Task time to go calculations

  double aat_tasktime_elapsed = Basic().Time - Calculated().TaskStartTime;
  double aat_tasklength_seconds = AATTaskLength*60;

  if (ActiveWayPoint==0) {
    if (Calculated().AATTimeToGo==0) {
      SetCalculated().AATTimeToGo = aat_tasklength_seconds;
    }
  } else if (aat_tasktime_elapsed>=0) {
    SetCalculated().AATTimeToGo = max(0,
				  aat_tasklength_seconds
				  - aat_tasktime_elapsed);
  }

  if(ValidTaskPoint(ActiveWayPoint) && (Calculated().AATTimeToGo>0)) {
    SetCalculated().AATMaxSpeed =
      Calculated().AATMaxDistance / Calculated().AATTimeToGo;
    SetCalculated().AATMinSpeed =
      Calculated().AATMinDistance / Calculated().AATTimeToGo;
    SetCalculated().AATTargetSpeed =
      Calculated().AATTargetDistance / Calculated().AATTimeToGo;
  }
}


void GlideComputerTask::AATStats_Distance() 
{
  int i;
  double MaxDistance, MinDistance, TargetDistance;

  mutexTaskData.Lock();

  MaxDistance = 0; MinDistance = 0; TargetDistance = 0;
  // Calculate Task Distances

  if(ValidTaskPoint(ActiveWayPoint))
    {
      i=ActiveWayPoint;

      double LegToGo=0, TargetLegToGo=0;

      if (i > 0 ) { //RLD only include distance from glider to next leg if we've started the task
        DistanceBearing(Basic().Latitude , Basic().Longitude ,
                        WayPointList[task_points[i].Index].Latitude,
                        WayPointList[task_points[i].Index].Longitude,
                        &LegToGo, NULL);

        DistanceBearing(Basic().Latitude , Basic().Longitude ,
                        task_points[i].AATTargetLat,
                        task_points[i].AATTargetLon,
                        &TargetLegToGo, NULL);

        if(task_points[i].AATType == CIRCLE)
        {
          MaxDistance = LegToGo + (task_points[i].AATCircleRadius );  // ToDo: should be adjusted for angle of max target and for national rules
          MinDistance = LegToGo - (task_points[i].AATCircleRadius );
        }
        else
        {
          MaxDistance = LegToGo + (task_points[i].AATSectorRadius );  // ToDo: should be adjusted for angle of max target.
          MinDistance = LegToGo;
        }

        TargetDistance = TargetLegToGo;
      }

      i++;
      while(ValidTaskPoint(i)) {
	double LegDistance, TargetLegDistance;

	DistanceBearing(WayPointList[task_points[i].Index].Latitude,
			WayPointList[task_points[i].Index].Longitude,
			WayPointList[task_points[i-1].Index].Latitude,
			WayPointList[task_points[i-1].Index].Longitude,
			&LegDistance, NULL);

	DistanceBearing(task_points[i].AATTargetLat,
			task_points[i].AATTargetLon,
			task_points[i-1].AATTargetLat,
			task_points[i-1].AATTargetLon,
			&TargetLegDistance, NULL);

	MaxDistance += LegDistance;
	MinDistance += LegDistance;

	if(task_points[ActiveWayPoint].AATType == CIRCLE) {
	  // breaking out single Areas increases accuracy for start
	  // and finish

	  // sector at start of (i)th leg
	  if (i-1 == 0) {// first leg of task
	    // add nothing
	    MaxDistance -= StartRadius; // e.g. Sports 2009 US Rules A116.3.2.  To Do: This should be configured multiple countries
	    MinDistance -= StartRadius;
	  } else { // not first leg of task
	    MaxDistance += (task_points[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	    MinDistance -= (task_points[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }

	  // sector at end of ith leg
	  if (!ValidTaskPoint(i+1)) {// last leg of task
	    // add nothing
	    MaxDistance -= FinishRadius; // To Do: This can be configured for finish rules
	    MinDistance -= FinishRadius;
	  } else { // not last leg of task
	    MaxDistance += (task_points[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	    MinDistance -= (task_points[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }
	} else { // not circle (pie slice)
	  // sector at start of (i)th leg
	  if (i-1 == 0) {// first leg of task
	    // add nothing
	    MaxDistance += 0; // To Do: This can be configured for start rules
	  } else { // not first leg of task
	    MaxDistance += (task_points[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }

	  // sector at end of ith leg
	  if (!ValidTaskPoint(i+1)) {// last leg of task
	    // add nothing
	    MaxDistance += 0; // To Do: This can be configured for finish rules
	  } else { // not last leg of task
	    MaxDistance += (task_points[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
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
  mutexTaskData.Unlock();
}


void GlideComputerTask::AATStats()
{
  if (!WayPointList
      || !AATEnabled
      || Calculated().ValidFinish) return ;

  AATStats_Distance();
  AATStats_Time();
}


void GlideComputerTask::CheckTransitionFinalGlide() {
  int FinalWayPoint = getFinalWaypoint();
  // update final glide mode status
  if (((ActiveWayPoint == FinalWayPoint)
       ||(ForceFinalGlide))
      && (ValidTaskPoint(ActiveWayPoint))) {

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
                ActiveWayPoint,
                Calculated().DistanceVario,
                Calculated().GPSVario);
      }
    }
}
#endif


bool GlideComputerTask::TaskAltitudeRequired(double this_maccready, double *Vfinal,
					     double *TotalTime, double *TotalDistance,
					     int *ifinal,
					     const double cruise_efficiency)
{
  int i;
  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;
  double LegTime, LegDistance, LegBearing, LegAltitude;
  bool retval = false;

  // Calculate altitude required from start of task

  bool isfinal=true;
  LegAltitude = 0;
  double TotalAltitude = 0;
  *TotalTime = 0; *TotalDistance = 0;
  *ifinal = 0;

  ScopeLock protect(mutexTaskData);

  double height_above_finish = FAIFinishHeight( 0)-
    FAIFinishHeight( -1);

  for(i=MAXTASKPOINTS-2;i>=0;i--) {


    if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

    w1lat = WayPointList[task_points[i].Index].Latitude;
    w1lon = WayPointList[task_points[i].Index].Longitude;
    w0lat = WayPointList[task_points[i+1].Index].Latitude;
    w0lon = WayPointList[task_points[i+1].Index].Longitude;

    if (AATEnabled) {
      w1lat = task_points[i].AATTargetLat;
      w1lon = task_points[i].AATTargetLon;
      if (!isfinal) {
        w0lat = task_points[i+1].AATTargetLat;
        w0lon = task_points[i+1].AATTargetLon;
      }
    }

    DistanceBearing(w1lat, w1lon,
                    w0lat, w0lon,
                    &LegDistance, &LegBearing);

    *TotalDistance += LegDistance;

    LegAltitude =
      GlidePolar::MacCreadyAltitude(this_maccready,
                                    LegDistance,
                                    LegBearing,
                                    Calculated().WindSpeed,
                                    Calculated().WindBearing,
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

    if (LegTime<0) {
      return false;
    } else {
      *TotalTime += LegTime;
    }
    if (isfinal) {
      *ifinal = i+1;
      if (LegTime>0) {
        *Vfinal = LegDistance/LegTime;
      }
    }
    isfinal = false;
  }

  if (*ifinal==0) {
    retval = false;
    goto OnExit;
  }

  TotalAltitude += FAIFinishHeight( -1);

  if (!ValidTaskPoint(*ifinal)) {
    SetCalculated().TaskAltitudeRequiredFromStart = TotalAltitude;
    retval = false;
  } else {
    SetCalculated().TaskAltitudeRequiredFromStart = TotalAltitude;
    retval = true;
  }
 OnExit:
  return retval;
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
  int ifinal;
  double TotalTime=0, TotalDistance=0, Vfinal=0;

  if (!ValidTaskPoint(ActiveWayPoint)) return;
  if (TaskIsTemporary()) return;
  if (Calculated().ValidFinish) return;
  if (!Calculated().Flying) return;

  // in case we leave early due to error
  SetCalculated().TaskSpeedAchieved = 0;
  SetCalculated().TaskSpeed = 0;

  if (ActiveWayPoint<=0) { // no task speed before start
    SetCalculated().TaskSpeedInstantaneous = 0;
    return;
  }

  ScopeLock protect(mutexTaskData);

  if (TaskAltitudeRequired(this_maccready, &Vfinal,
                           &TotalTime, &TotalDistance, &ifinal, 
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

    double h1 = max(0,Calculated().NavAltitude-hf);
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
    double hx = max(0,SpeedHeight());
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

    double dc = max(0,dr-dFinal);
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
    if (isTaskDeclared()) {
      konst = 1.0;
    } else {
      konst = 1.1;
    }

    double termikLigaPoints = 0;
    if (d1 > 0) {
      termikLigaPoints = konst*(0.015*0.001*d1-(400.0/(0.001*d1))+12.0)*v1*3.6*100.0/(double)SettingsComputer().Handicap;
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

      double ttg = max(1,Calculated().LegTimeToGo);
      //      double Vav = d0/max(1.0,t0);
      double Vrem = Calculated().LegDistanceToGo/ttg;
      double Vref = // Vav;
	Vrem;
      double sr = -GlidePolar::SinkRate(Vstar);
      double height_diff = max(0,-Calculated().TaskAltitudeDifference);

      if (Calculated().timeCircling>30) {
	mc_safe = max(this_maccready,
		      Calculated().TotalHeightClimb/Calculated().timeCircling);
      }
      // circling percentage during cruise/climb
      double rho_cruise = max(0.0,min(1.0,mc_safe/(sr+mc_safe)));
      double rho_climb = 1.0-rho_cruise;
      double time_climb = height_diff/mc_safe;

      // calculate amount of time in cruise/climb glide
      double rho_c = max(0,min(1,time_climb/ttg));

      if (Calculated().FinalGlide) {
	if (rho_climb>0) {
	  rho_c = max(0,min(1,rho_c/rho_climb));
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
        if ((ActiveWayPoint==LastCalculated().ActiveWayPoint)
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
			      max(0,min(100.0,tsi_av))));

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

    double lat, lon;
    bool out_of_range;
    double distance_soarable =
      FinalGlideThroughTerrain(LegBearing,
                               &Basic(), &Calculated(),
			       SettingsComputer(),
                               &lat,
                               &lon,
                               LegToGo, &out_of_range, NULL);

    if ((!out_of_range)&&(distance_soarable< LegToGo)) {
      SetCalculated().TerrainWarningLatitude = lat;
      SetCalculated().TerrainWarningLongitude = lon;
    } else {
      SetCalculated().TerrainWarningLatitude = 0.0;
      SetCalculated().TerrainWarningLongitude = 0.0;
    }
  } else {
    SetCalculated().TerrainWarningLatitude = 0.0;
    SetCalculated().TerrainWarningLongitude = 0.0;
  }
}


void 
GlideComputerTask::ResetEnter()
{
  aatdistance.ResetEnterTrigger(ActiveWayPoint);
}



void
GlideComputerTask::DoAutoMacCready(double mc_setting)
{
  bool is_final_glide = false;

  if (!SettingsComputer().AutoMacCready) return;

  ScopeLock protect(mutexTaskData);

  double mc_new = mc_setting;
  static bool first_mc = true;

  if (Calculated().FinalGlide && ActiveIsFinalWaypoint()) {
    is_final_glide = true;
  } else {
    first_mc = true;
  }

  if (!ValidTaskPoint(ActiveWayPoint)) {
    if (Calculated().AdjustedAverageThermal>0) {
      mc_new = Calculated().AdjustedAverageThermal;
    }
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
	 - FAIFinishHeight(ActiveWayPoint))/
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
  double lat, lon;
  bool out_of_range;
  double distance_soarable =
    FinalGlideThroughTerrain(LegBearing,
                             Basic, Calculated,
			     settings,
                             &lat,
                             &lon,
                             LegToGo, &out_of_range, NULL);

  if ((out_of_range)||(distance_soarable> LegToGo)) {
    return true;
  } else {
    return false;
  }
}


void
GlideComputerTask::CalculateWaypointReachable(void)
{
  unsigned int i;
  double WaypointDistance, WaypointBearing, 
    AltitudeRequired,AltitudeDifference;

  SetCalculated().LandableReachable = false;

  if (!WayPointList) return;

  ScopeLock protect(mutexTaskData);

  for(i=0;i<NumberOfWayPoints;i++) {
    if ((WayPointList[i].Visible &&
	 (
	  ((WayPointList[i].Flags & AIRPORT) == AIRPORT) ||
	  ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT)
	  ))
	|| WaypointInTask(i) ) {

      DistanceBearing(Basic().Latitude,
		      Basic().Longitude,
		      WayPointList[i].Latitude,
		      WayPointList[i].Longitude,
		      &WaypointDistance,
		      &WaypointBearing);

      AltitudeRequired =
	GlidePolar::MacCreadyAltitude
	(GlidePolar::SafetyMacCready,
	 WaypointDistance,
	 WaypointBearing,
	 Calculated().WindSpeed,
	 Calculated().WindBearing,
	 0,0,true,0);
      AltitudeRequired = AltitudeRequired + 
	SettingsComputer().SAFETYALTITUDEARRIVAL
	+ WayPointList[i].Altitude ;
      AltitudeDifference = Calculated().NavAltitude - AltitudeRequired;
      WayPointList[i].AltArivalAGL = AltitudeDifference;

      if(AltitudeDifference >=0){
	WayPointList[i].Reachable = TRUE;
	if (!Calculated().LandableReachable || ((int)i==ActiveWayPoint)) {
	  if (CheckLandableReachableTerrain(&Basic(),
					    &Calculated(),
					    SettingsComputer(),
					    WaypointDistance,
					    WaypointBearing)) {
	    SetCalculated().LandableReachable = true;
	  } else if ((int)i==ActiveWayPoint) {
	    WayPointList[i].Reachable = FALSE;
	  }
	}
      } else {
	WayPointList[i].Reachable = FALSE;
      }
    }
  }

  if (!Calculated().LandableReachable) {
    // widen search to far visible waypoints
    // (only do this if can't see one at present)

    for(i=0;i<NumberOfWayPoints;i++)
      {
        if(!WayPointList[i].Visible && WayPointList[i].FarVisible)
          // visible but only at a distance (limit this to 100km radius)
          {
            if(  ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
                 || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT) )
              {
                DistanceBearing(Basic().Latitude,
                                Basic().Longitude,
                                WayPointList[i].Latitude,
                                WayPointList[i].Longitude,
                                &WaypointDistance,
                                &WaypointBearing);

                if (WaypointDistance<100000.0) {
                  AltitudeRequired =
                    GlidePolar::MacCreadyAltitude
                    (GlidePolar::SafetyMacCready,
                     WaypointDistance,
                     WaypointBearing,
                     Calculated().WindSpeed,
                     Calculated().WindBearing,
                     0,0,true,0);

                  AltitudeRequired = AltitudeRequired + 
		    SettingsComputer().SAFETYALTITUDEARRIVAL
                    + WayPointList[i].Altitude ;
                  AltitudeDifference = Calculated().NavAltitude - AltitudeRequired;
                  WayPointList[i].AltArivalAGL = AltitudeDifference;

                  if(AltitudeDifference >=0){
                    WayPointList[i].Reachable = TRUE;
                    if (!Calculated().LandableReachable) {
                      if (CheckLandableReachableTerrain(&Basic(),
                                                        &Calculated(),
							SettingsComputer(),
                                                        WaypointDistance,
                                                        WaypointBearing)) {
                        SetCalculated().LandableReachable = true;
                      }
                    }
                  } else {
                    WayPointList[i].Reachable = FALSE;
                  }
                }
              }
          }
      }
  }
}
