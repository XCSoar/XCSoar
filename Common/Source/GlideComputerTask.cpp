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

#include "GlideComputer.hpp"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "Task.h"
#include "SettingsTask.hpp"
#include "WayPoint.hpp"
#include "Math/Earth.hpp"
#include "Math/Geometry.hpp"
#include "Math/Pressure.h"
#include "Math/LowPassFilter.hpp"
#include "McReady.h"
#include "GlideRatio.hpp"
#include "CalculationsTerrain.hpp"

#include "InputEvents.h"
// JMW TODO: abstract up to higher layer so a base copy of this won't 
// call any event

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

bool GlideComputerTask::DoLogging() {
  static double OLCLastTime = 0;
  double dtOLC = 5.0;

  if(gps_info.Time <= OLCLastTime) {
    OLCLastTime = gps_info.Time;
  }
  if (calculated_info.Flying) {
    if (calculated_info.Flying && (gps_info.Time - OLCLastTime >= dtOLC)) {
      mutexGlideComputer.Lock();
      bool restart = olc.addPoint(gps_info.Longitude,
			     gps_info.Latitude,
			     calculated_info.NavAltitude,
			     calculated_info.WaypointBearing,
			     gps_info.Time-calculated_info.TakeOffTime);
      mutexGlideComputer.Unlock();

      if (restart && EnableOLC) {
	calculated_info.ValidFinish = false;
	StartTask(false, false);
	calculated_info.ValidStart = true;
      }
      OLCLastTime += dtOLC;
      return true;
    }
  }
  return false;
}


// VENTA3 added radial
void GlideComputerTask::DistanceToHome() {
  int home_waypoint = HomeWaypoint;

  if (!ValidWayPoint(home_waypoint)) {
    calculated_info.HomeDistance = 0.0;
    calculated_info.HomeRadial = 0.0; // VENTA3
    return;
  }

  double w1lat = WayPointList[home_waypoint].Latitude;
  double w1lon = WayPointList[home_waypoint].Longitude;
  double w0lat = gps_info.Latitude;
  double w0lon = gps_info.Longitude;

  DistanceBearing(w1lat, w1lon,
                  w0lat, w0lon,
                  &calculated_info.HomeDistance, &calculated_info.HomeRadial);

}


void GlideComputerTask::DistanceToNext()
{
  mutexTaskData.Lock();

  if(ValidTaskPoint(ActiveWayPoint))
    {
      double w1lat, w1lon;
      double w0lat, w0lon;

      w0lat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
      w0lon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
      DistanceBearing(gps_info.Latitude, gps_info.Longitude,
                      w0lat, w0lon,
                      &calculated_info.WaypointDistance,
                      &calculated_info.WaypointBearing);

      calculated_info.ZoomDistance = calculated_info.WaypointDistance;

      if (AATEnabled && !TaskIsTemporary()
	  && (ActiveWayPoint>0) &&
          ValidTaskPoint(ActiveWayPoint+1)) {

        w1lat = Task[ActiveWayPoint].AATTargetLat;
        w1lon = Task[ActiveWayPoint].AATTargetLon;

        DistanceBearing(gps_info.Latitude, gps_info.Longitude,
                        w1lat, w1lon,
                        &calculated_info.WaypointDistance,
                        &calculated_info.WaypointBearing);

        if (calculated_info.WaypointDistance>AATCloseDistance()*3.0) {
          calculated_info.ZoomDistance = max(calculated_info.WaypointDistance,
                                         calculated_info.ZoomDistance);
        } else {
	  calculated_info.WaypointBearing = AATCloseBearing();
        }

      } else if ((ActiveWayPoint==0) && (ValidTaskPoint(ActiveWayPoint+1))
                 && (calculated_info.IsInSector) &&
		 !TaskIsTemporary()) {

        // JMW set waypoint bearing to start direction if in start sector

        if (AATEnabled) {
          w1lat = Task[ActiveWayPoint+1].AATTargetLat;
          w1lon = Task[ActiveWayPoint+1].AATTargetLon;
        } else {
          w1lat = WayPointList[Task[ActiveWayPoint+1].Index].Latitude;
          w1lon = WayPointList[Task[ActiveWayPoint+1].Index].Longitude;
        }

        DistanceBearing(gps_info.Latitude, gps_info.Longitude,
                        w1lat, w1lon,
                        NULL,
                        &calculated_info.WaypointBearing);
      }
    }
  else
    {
      calculated_info.ZoomDistance = 0;
      calculated_info.WaypointDistance = 0;
      calculated_info.WaypointBearing = 0;
    }
  mutexTaskData.Unlock();
}


void GlideComputerTask::AltitudeRequired(const double this_maccready,
					 const double cruise_efficiency)
{
  mutexTaskData.Lock();
  if(ValidTaskPoint(ActiveWayPoint))
    {
      double wp_alt = FAIFinishHeight(ActiveWayPoint);
      double height_above_wp =
	calculated_info.NavAltitude + calculated_info.EnergyHeight
	- wp_alt;

      calculated_info.NextAltitudeRequired =
        GlidePolar::MacCreadyAltitude(this_maccready,
                        calculated_info.WaypointDistance,
                        calculated_info.WaypointBearing,
                        calculated_info.WindSpeed, calculated_info.WindBearing,
                        0, 0,
			true,
			NULL, height_above_wp, cruise_efficiency
                        );
      // JMW CHECK FGAMT

	// VENTA6
	if (this_maccready==0 ) calculated_info.NextAltitudeRequired0=calculated_info.NextAltitudeRequired;
        else
	      calculated_info.NextAltitudeRequired0 =
		GlidePolar::MacCreadyAltitude(0,
				calculated_info.WaypointDistance,
				calculated_info.WaypointBearing,
				calculated_info.WindSpeed, calculated_info.WindBearing,
				0, 0,
				true,
				NULL, height_above_wp, cruise_efficiency
				);



      calculated_info.NextAltitudeRequired += wp_alt;
      calculated_info.NextAltitudeRequired0 += wp_alt; // VENTA6

      calculated_info.NextAltitudeDifference =
        calculated_info.NavAltitude
        + calculated_info.EnergyHeight
        - calculated_info.NextAltitudeRequired;

      calculated_info.NextAltitudeDifference0 =
        calculated_info.NavAltitude
        + calculated_info.EnergyHeight
        - calculated_info.NextAltitudeRequired0;
    }
  else
    {
      calculated_info.NextAltitudeRequired = 0;
      calculated_info.NextAltitudeDifference = 0;
      calculated_info.NextAltitudeDifference0 = 0; // VENTA6
    }
  mutexTaskData.Unlock();
}


double GlideComputerTask::AATCloseBearing() {
  // ensure waypoint goes in direction of track if very close
  double course_bearing;
  DistanceBearing(Task[ActiveWayPoint-1].AATTargetLat,
		  Task[ActiveWayPoint-1].AATTargetLon,
		  gps_info.Latitude,
		  gps_info.Longitude,
		  NULL, &course_bearing);

  course_bearing = AngleLimit360(course_bearing+
				 Task[ActiveWayPoint].AATTargetOffsetRadial);
  return course_bearing;
}


double GlideComputerTask::FAIFinishHeight(int wp) {
  int FinalWayPoint = getFinalWaypoint();
  if (wp== -1) {
    wp = FinalWayPoint;
  }
  double wp_alt;
  if(ValidTaskPoint(wp)) {
    wp_alt = WayPointList[Task[wp].Index].Altitude;
  } else {
    wp_alt = 0;
  }

  if (!TaskIsTemporary() && (wp==FinalWayPoint)) {
    if (EnableFAIFinishHeight && !AATEnabled) {
      return max(max(FinishMinHeight, SAFETYALTITUDEARRIVAL)+ wp_alt,
                 calculated_info.TaskStartAltitude-1000.0);
    } else {
      return max(FinishMinHeight, SAFETYALTITUDEARRIVAL)+wp_alt;
    }
  } else {
    return wp_alt + SAFETYALTITUDEARRIVAL;
  }
}


bool GlideComputerTask::InTurnSector(const int the_turnpoint)
{
  double AircraftBearing;

  if (!ValidTaskPoint(the_turnpoint)) return false;

  if(SectorType==0)
    {
      if(calculated_info.WaypointDistance < SectorRadius)
        {
          return true;
        }
    }
  if (SectorType>0)
    {
      mutexTaskData.Lock();
      DistanceBearing(WayPointList[Task[the_turnpoint].Index].Latitude,
                      WayPointList[Task[the_turnpoint].Index].Longitude,
                      gps_info.Latitude ,
                      gps_info.Longitude,
                      NULL, &AircraftBearing);
      mutexTaskData.Unlock();

      AircraftBearing = AircraftBearing - Task[the_turnpoint].Bisector ;
      while (AircraftBearing<-180) {
        AircraftBearing+= 360;
      }
      while (AircraftBearing>180) {
        AircraftBearing-= 360;
      }

      if (SectorType==2) {
        // JMW added german rules
        if (calculated_info.WaypointDistance<500) {
          return true;
        }
      }
      if( (AircraftBearing >= -45) && (AircraftBearing <= 45))
        {
          if (SectorType==1) {
            if(calculated_info.WaypointDistance < SectorRadius)
              {
                return true;
              }
          } else {
            // JMW added german rules
            if(calculated_info.WaypointDistance < 10000)
              {
                return true;
              }
          }
        }
    }
  return false;
}

bool InAATTurnSector(const double longitude, const double latitude,
                    const int the_turnpoint)
{
  double AircraftBearing;
  bool retval = false;

  if (!ValidTaskPoint(the_turnpoint)) {
    return false;
  }

  double distance;
  mutexTaskData.Lock();
  DistanceBearing(WayPointList[Task[the_turnpoint].Index].Latitude,
                  WayPointList[Task[the_turnpoint].Index].Longitude,
                  latitude,
                  longitude,
                  &distance, &AircraftBearing);

  if(Task[the_turnpoint].AATType ==  CIRCLE) {
    if(distance < Task[the_turnpoint].AATCircleRadius) {
      retval = true;
    }
  } else if(distance < Task[the_turnpoint].AATSectorRadius) {
    if (AngleInRange(Task[the_turnpoint].AATStartRadial,
                     Task[the_turnpoint].AATFinishRadial,
                     AngleLimit360(AircraftBearing), true)) {
      retval = true;
    }
  }

  mutexTaskData.Unlock();
  return retval;
}


bool GlideComputerTask::ValidFinish( ) 
{
  if ((FinishMinHeight>0)
      &&(calculated_info.TerrainValid)
      &&(calculated_info.AltitudeAGL<FinishMinHeight)) {
    return false;
  } else {
    return true;
  }
}


bool GlideComputerTask::InFinishSector(const int i)
{
  static int LastInSector = FALSE;
  double AircraftBearing;
  double FirstPointDistance;
  bool retval = false;

  if (!WayPointList) return FALSE;

  if (!ValidFinish()) return FALSE;

  // Finish invalid
  if (!ValidTaskPoint(i)) return FALSE;

  mutexTaskData.Lock();

  // distance from aircraft to start point
  DistanceBearing(gps_info.Latitude,
                  gps_info.Longitude,
                  WayPointList[Task[i].Index].Latitude,
                  WayPointList[Task[i].Index].Longitude,
                  &FirstPointDistance,
                  &AircraftBearing);
  bool inrange = false;
  inrange = (FirstPointDistance<FinishRadius);
  if (!inrange) {
    LastInSector = false;
  }

  if(!FinishLine) // Start Circle
    {
      retval = inrange;
      goto OnExit;
    }

  // Finish line
  AircraftBearing = AngleLimit180(AircraftBearing - Task[i].InBound);

  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  if(FinishLine==1) { // Finish line
    approaching = ((AircraftBearing >= -90) && (AircraftBearing <= 90));
  } else {
    // FAI 90 degree
    approaching = !((AircraftBearing >= 135) || (AircraftBearing <= -135));
  }

  if (inrange) {

    if (LastInSector) {
      // previously approaching the finish line
      if (!approaching) {
        // now moving away from finish line
        LastInSector = false;
        retval = TRUE;
        goto OnExit;
      }
    } else {
      if (approaching) {
        // now approaching the finish line
        LastInSector = true;
      }
    }

  } else {
    LastInSector = false;
  }
 OnExit:
  mutexTaskData.Unlock();
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

bool GlideComputerTask::ValidStartSpeed(const DWORD Margin) {
  bool valid = true;
  if (StartMaxSpeed!=0) {
    if (gps_info.AirspeedAvailable) {
      if (gps_info.IndicatedAirspeed>(StartMaxSpeed+Margin))
        valid = false;
    } else {
      if (gps_info.Speed>(StartMaxSpeed+Margin))
        valid = false;
    }
  }
  return valid;
}


bool GlideComputerTask::InsideStartHeight(const DWORD Margin) {
  bool valid = true;
  if ((StartMaxHeight!=0)&&(calculated_info.TerrainValid)) {
    if (StartHeightRef == 0) {
      if (calculated_info.AltitudeAGL>(StartMaxHeight+Margin))
	valid = false;
    } else {
      if (calculated_info.NavAltitude>(StartMaxHeight+Margin))
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
  DistanceBearing(gps_info.Latitude,
                  gps_info.Longitude,
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


bool GlideComputerTask::InStartSector(int &index,
				      BOOL *CrossedStart)
{
  static bool LastInSector = false;
  static int EntryStartSector = index;

  bool isInSector= false;
  bool retval=false;

  if (!calculated_info.Flying ||
      !ValidTaskPoint(ActiveWayPoint) ||
      !ValidTaskPoint(0))
    return false;

  mutexTaskData.Lock();

  bool in_height = true;

  if ((ActiveWayPoint>0)
      && !ValidTaskPoint(ActiveWayPoint+1)) {
    // don't detect start if finish is selected
    retval = false;
    goto OnExit;
  }

// ToLo: do "soft" check for height only
  in_height = InsideStartHeight(StartMaxHeightMargin);

  if ((Task[0].Index != EntryStartSector) && (EntryStartSector>=0)) {
    LastInSector = false;
    EntryStartSector = Task[0].Index;
  }

  isInSector = InStartSector_Internal(Task[0].Index, Task[0].OutBound,
                                      LastInSector);
  isInSector &= in_height;

  *CrossedStart = LastInSector && !isInSector;
  LastInSector = isInSector;
  if (*CrossedStart) {
    goto OnExit;
  }

  if (EnableMultipleStartPoints) {
    for (int i=0; i<MAXSTARTPOINTS; i++) {
      if (StartPoints[i].Active && (StartPoints[i].Index>=0)
          && (StartPoints[i].Index != Task[0].Index)) {

        retval = InStartSector_Internal(StartPoints[i].Index,
                                        StartPoints[i].OutBound,
                                        StartPoints[i].InSector);
	retval &= in_height;
        isInSector |= retval;

        index = StartPoints[i].Index;
        *CrossedStart = StartPoints[i].InSector && !retval;
        StartPoints[i].InSector = retval;
        if (*CrossedStart) {
          if (Task[0].Index != index) {
            Task[0].Index = index;
            LastInSector = false;
            EntryStartSector = index;
            RefreshTask();
          }
          goto OnExit;
        }

      }
    }
  }

 OnExit:

  mutexTaskData.Unlock();
  return isInSector;
}

#define AUTOADVANCE_MANUAL 0
#define AUTOADVANCE_AUTO 1
#define AUTOADVANCE_ARM 2
#define AUTOADVANCE_ARMSTART 3

bool GlideComputerTask::ReadyToStart() {
  if (!calculated_info.Flying) {
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
  static int lastReady = -1;
  static int lastActive = -1;
  bool say_ready = false;

  // 0: Manual
  // 1: Auto
  // 2: Arm
  // 3: Arm start

  if (!calculated_info.Flying) {
    lastReady = -1;
    lastActive = -1;
    return false;
  }

  if (AutoAdvance== AUTOADVANCE_AUTO) {
    if (reset) AdvanceArmed = false;
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
    if ((ActiveWayPoint == 0) || restart) {
      if (!AdvanceArmed) {
        say_ready = true;
      } else if (reset) {
        AdvanceArmed = false;
        return true;
      }
    } else {
      // JMW fixed 20070528
      if (ActiveWayPoint>0) {
        if (reset) AdvanceArmed = false;
        return true;
      }
    }
  }

  // see if we've gone back a waypoint (e.g. restart)
  if (ActiveWayPoint < lastActive) {
    lastReady = -1;
  }
  lastActive = ActiveWayPoint;

  if (say_ready) {
    if (ActiveWayPoint != lastReady) {
      InputEvents::processGlideComputer(GCE_ARM_READY);
      lastReady = ActiveWayPoint;
    }
  }
  return false;
}


void GlideComputerTask::CheckStart(int *LastStartSector) {
  BOOL StartCrossed= false;

  if (InStartSector(*LastStartSector, &StartCrossed)) {
    calculated_info.IsInSector = true;

    if (ReadyToStart()) {
      aatdistance.AddPoint(gps_info.Longitude,
			   gps_info.Latitude,
			   0);
    }
    // ToLo: we are ready to start even when outside start rules but within margin
    if (ValidStartSpeed(StartMaxSpeedMargin)) {
      ReadyToAdvance(false, true);
    }
    // TODO accuracy: monitor start speed throughout time in start sector
  }
  if (StartCrossed) {
    // ToLo: Check weather speed and height are within the rules or not (zero margin)
    if(!IsFinalWaypoint() && ValidStartSpeed() && InsideStartHeight()) {

      // This is set whether ready to advance or not, because it will
      // appear in the flight log, so if it's valid, it's valid.
      calculated_info.ValidStart = true;

      if (ReadyToAdvance(true, true)) {
        ActiveWayPoint=0; // enforce this since it may be 1
        StartTask(true, true);
      }
      if (calculated_info.Flying) {
        calculated_info.ValidFinish = false;
      }
      // JMW TODO accuracy: This causes Vaverage to go bonkers
      // if the user has already passed the start
      // but selects the start

      // Note: pilot must have armed advance
      // for the start to be registered

    // ToLo: If speed and height are outside the rules they must be within the margin...
    } else {

      if ((ActiveWayPoint<=1)
          && !IsFinalWaypoint()
          && (calculated_info.ValidStart==false)
          && (calculated_info.Flying)) {

        // need to detect bad starts, just to get the statistics
        // in case the bad start is the best available, or the user
        // manually started
        StartTask(false, false);
//        calculated_info.ValidStart = false;

        bool startTaskAnyway = false;

        if (ReadyToAdvance(true, true)) {

	  /* JMW TODO THIS IS BAD!!! SEND AN EVENT TO THE GUI INSTEAD
	     OF RUNNING A DIALOG FROM THE CALCULATIONS THREAD 
          //DoStatusMessage(TEXT("Start Anyway?"));
          dlgStartTaskShowModal(&startTaskAnyway,
                                calculated_info.TaskStartTime,
                                calculated_info.TaskStartSpeed,
                                calculated_info.TaskStartAltitude);
	  */
          if (startTaskAnyway) {
            ActiveWayPoint=0; // enforce this since it may be 1
            StartTask(true, true);
          }
        }

        calculated_info.ValidStart = startTaskAnyway;

        if (calculated_info.Flying) {
          calculated_info.ValidFinish = false;
        }

	// TODO: Display infobox when only a bit over start rules
      }

    }
  }
}


BOOL GlideComputerTask::CheckRestart(int *LastStartSector) 
{
  if((gps_info.Time - calculated_info.TaskStartTime < 3600)
     &&(ActiveWayPoint<=1)) {

    /*
    BOOL StartCrossed;
    if(InStartSector(*LastStartSector, &StartCrossed)) {
      calculated_info.IsInSector = true;

      // this allows restart if returned to start sector before
      // 10 minutes after task start
      ActiveWayPoint = 0;
      return TRUE;
    }
    */
    CheckStart(LastStartSector);
  }
  return FALSE;
}


void GlideComputerTask::CheckFinish( ) {
  if (InFinishSector(ActiveWayPoint)) {
    calculated_info.IsInSector = true;
    aatdistance.AddPoint(gps_info.Longitude,
                         gps_info.Latitude,
                         ActiveWayPoint);
    if (!calculated_info.ValidFinish) {
      calculated_info.ValidFinish = true;
      AnnounceWayPointSwitch(false);
      SaveFinish();
    }
  }
}


void GlideComputerTask::AddAATPoint(int taskwaypoint) {
  bool insector = false;
  if (taskwaypoint>0) {
    if (AATEnabled) {
      insector = InAATTurnSector(gps_info.Longitude,
                                 gps_info.Latitude, taskwaypoint);
    } else {
      insector = InTurnSector(taskwaypoint);
    }
    if(insector) {
      if (taskwaypoint == ActiveWayPoint) {
        calculated_info.IsInSector = true;
      }
      aatdistance.AddPoint(gps_info.Longitude,
                           gps_info.Latitude,
                           taskwaypoint);
    }
  }
}


void GlideComputerTask::CheckInSector() {

  if (ActiveWayPoint>0) {
    AddAATPoint(ActiveWayPoint-1);
  }
  AddAATPoint(ActiveWayPoint);

  // JMW Start bug XXX

  mutexGlideComputer.Lock();

  if (aatdistance.HasEntered(ActiveWayPoint)) {
    if (ReadyToAdvance(true, false)) {
      AnnounceWayPointSwitch(true);
    }
    if (calculated_info.Flying) {
      calculated_info.ValidFinish = false;
    }
  }
  mutexGlideComputer.Unlock();
}


void GlideComputerTask::InSector()
{
  static int LastStartSector = -1;

  if (ActiveWayPoint<0) return;

  mutexTaskData.Lock();

  calculated_info.IsInSector = false;

  if(ActiveWayPoint == 0) {
    CheckStart(&LastStartSector);
  } else {
    if(IsFinalWaypoint()) {
      LastStartSector = -1;
      AddAATPoint(ActiveWayPoint-1);
      CheckFinish();
    } else {
      CheckRestart(&LastStartSector);
      if (ActiveWayPoint>0) {
        CheckInSector();
        LastStartSector = -1;
      }
    }
  }
  mutexTaskData.Unlock();
}



////

void GlideComputerTask::LDNext(const double LegToGo) {
  double height_above_leg = calculated_info.NavAltitude+calculated_info.EnergyHeight
    - FAIFinishHeight(ActiveWayPoint);

  calculated_info.LDNext = UpdateLD(calculated_info.LDNext,
                                LegToGo,
                                height_above_leg,
                                0.5);
}


void GlideComputerTask::CheckForceFinalGlide() {
  // Auto Force Final Glide forces final glide mode
  // if above final glide...
  if (TaskAborted) {
    ForceFinalGlide = false;
  } else {
    if (AutoForceFinalGlide) {
      if (!calculated_info.FinalGlide) {
        if (calculated_info.TaskAltitudeDifference>120) {
          ForceFinalGlide = true;
        } else {
          ForceFinalGlide = false;
        }
      } else {
        if (calculated_info.TaskAltitudeDifference<-120) {
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

    calculated_info.LegSpeed = 0;
    calculated_info.LegDistanceToGo = 0;
    calculated_info.LegDistanceCovered = 0;
    calculated_info.LegTimeToGo = 0;

    if (!AATEnabled) {
      calculated_info.AATTimeToGo = 0;
    }

    //    calculated_info.TaskSpeed = 0;

    calculated_info.TaskDistanceToGo = 0;
    calculated_info.TaskDistanceCovered = 0;
    calculated_info.TaskTimeToGo = 0;
    calculated_info.TaskTimeToGoTurningNow = -1;

    calculated_info.TaskAltitudeRequired = 0;
    calculated_info.TaskAltitudeDifference = 0;
    calculated_info.TaskAltitudeDifference0 = 0;

    calculated_info.TerrainWarningLatitude = 0.0;
    calculated_info.TerrainWarningLongitude = 0.0;

    calculated_info.LDFinish = INVALID_GR;
    calculated_info.GRFinish = INVALID_GR; // VENTA-ADDON
    calculated_info.LDNext = INVALID_GR;

    calculated_info.FinalGlide = 0;
    CheckFinalGlideThroughTerrain(0.0, 0.0);

    // no task selected, so work things out at current heading

    GlidePolar::MacCreadyAltitude(this_maccready, 100.0,
                                  gps_info.TrackBearing,
                                  calculated_info.WindSpeed,
                                  calculated_info.WindBearing,
                                  &(calculated_info.BestCruiseTrack),
                                  &(calculated_info.VMacCready),
                                  (calculated_info.FinalGlide==1),
                                  NULL, 1.0e6, cruise_efficiency);

    return;
  }

  mutexTaskData.Lock();

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
    w1lat = Task[ActiveWayPoint].AATTargetLat;
    w1lon = Task[ActiveWayPoint].AATTargetLon;
  } else {
    w1lat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
    w1lon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
  }

  DistanceBearing(gps_info.Latitude,
                  gps_info.Longitude,
                  w1lat,
                  w1lon,
                  &LegToGo, &LegBearing);

  if (AATEnabled && (ActiveWayPoint>0) && ValidTaskPoint(ActiveWayPoint+1)
      && calculated_info.IsInSector && (this_maccready>0.1) &&
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
      w0lat = Task[ActiveWayPoint-1].AATTargetLat;
      w0lon = Task[ActiveWayPoint-1].AATTargetLon;
    } else {
      w0lat = WayPointList[Task[ActiveWayPoint-1].Index].Latitude;
      w0lon = WayPointList[Task[ActiveWayPoint-1].Index].Longitude;
    }

    DistanceBearing(w1lat,
                    w1lon,
                    w0lat,
                    w0lon,
                    &LegDistance, NULL);

    LegCovered = ProjectedDistance(w0lon, w0lat,
                                   w1lon, w1lat,
                                   gps_info.Longitude,
                                   gps_info.Latitude);

    if ((StartLine==0) && (ActiveWayPoint==1)) {
      // Correct speed calculations for radius
      // JMW TODO accuracy: legcovered replace this with more accurate version
      // LegDistance -= StartRadius;
      LegCovered = max(0,LegCovered-StartRadius);
    }
  }

  calculated_info.LegDistanceToGo = LegToGo;
  calculated_info.LegDistanceCovered = LegCovered;
  calculated_info.TaskDistanceCovered = LegCovered;

  if (gps_info.Time > calculated_info.LegStartTime) {
    mutexGlideComputer.Lock();
    if (flightstats.LegStartTime[ActiveWayPoint]<0) {
      flightstats.LegStartTime[ActiveWayPoint] = gps_info.Time;
    }
    mutexGlideComputer.Unlock();
    calculated_info.LegSpeed = calculated_info.LegDistanceCovered
      / (gps_info.Time - calculated_info.LegStartTime);
  }

  ///////////////////////////////////////////////////
  // Now add distances for start to previous waypoint

  if (!TaskIsTemporary()) {

    if (!AATEnabled) {
      for(int i=0;i< ActiveWayPoint-1; i++)
        {
          if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

          w1lat = WayPointList[Task[i].Index].Latitude;
          w1lon = WayPointList[Task[i].Index].Longitude;
          w0lat = WayPointList[Task[i+1].Index].Latitude;
          w0lon = WayPointList[Task[i+1].Index].Longitude;

          DistanceBearing(w1lat,
                          w1lon,
                          w0lat,
                          w0lon,
                          &LegDistance, NULL);
          calculated_info.TaskDistanceCovered += LegDistance;
        }
    } else if (ActiveWayPoint>0) {
      // JMW added correction for distance covered
      mutexGlideComputer.Lock();
      calculated_info.TaskDistanceCovered =
        aatdistance.DistanceCovered(gps_info.Longitude,
                                    gps_info.Latitude,
                                    ActiveWayPoint);
      mutexGlideComputer.Unlock();
    }
  }

  ///////////////////////////////////////////////////////////

  CheckTransitionFinalGlide();

  // accumulators
  double TaskAltitudeRequired = 0;
  double TaskAltitudeRequired0 = 0;
  calculated_info.TaskDistanceToGo = 0;
  calculated_info.TaskTimeToGo = 0;
  calculated_info.TaskTimeToGoTurningNow = 0;

  double LegTime0;

  // Calculate Final Glide To Finish

  int FinalWayPoint = getFinalWaypoint();

  double height_above_finish = calculated_info.NavAltitude+
    calculated_info.EnergyHeight-FAIFinishHeight(-1);

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
	w1lat = Task[task_index].AATTargetLat;
	w1lon = Task[task_index].AATTargetLon;
	w0lat = Task[task_index-1].AATTargetLat;
	w0lon = Task[task_index-1].AATTargetLon;
      } else {
	w1lat = WayPointList[Task[task_index].Index].Latitude;
	w1lon = WayPointList[Task[task_index].Index].Longitude;
	w0lat = WayPointList[Task[task_index-1].Index].Latitude;
	w0lon = WayPointList[Task[task_index-1].Index].Longitude;
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
			  calculated_info.WindSpeed,
			  calculated_info.WindBearing,
			  0, 0,
			  this_is_final,
			  &this_LegTimeToGo,
			  height_above_finish, cruise_efficiency);

      double LegAltitude0 = GlidePolar::
	MacCreadyAltitude(0,
			  NextLegDistance, NextLegBearing,
			  calculated_info.WindSpeed,
			  calculated_info.WindBearing,
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

      calculated_info.TaskDistanceToGo += NextLegDistance;
      calculated_info.TaskTimeToGo += this_LegTimeToGo;

      if (task_index==1) {
	StartBestCruiseTrack = NextLegBearing;
      }

      if (calc_turning_now) {
	if (task_index == ActiveWayPoint+1) {

	  double NextLegDistanceTurningNow, NextLegBearingTurningNow;
	  double this_LegTimeToGo_turningnow=0;

	  DistanceBearing(gps_info.Latitude,
			  gps_info.Longitude,
			  w1lat,
			  w1lon,
			  &NextLegDistanceTurningNow,
			  &NextLegBearingTurningNow);

	  GlidePolar::
	    MacCreadyAltitude(this_maccready,
			      NextLegDistanceTurningNow,
			      NextLegBearingTurningNow,
			      calculated_info.WindSpeed,
			      calculated_info.WindBearing,
			      0, 0,
			      this_is_final,
			      &this_LegTimeToGo_turningnow,
			      height_above_finish, cruise_efficiency);
	  calculated_info.TaskTimeToGoTurningNow += this_LegTimeToGo_turningnow;
	} else {
	  calculated_info.TaskTimeToGoTurningNow += this_LegTimeToGo;
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
      ValidTaskPoint(ActiveWayPoint+1) && calculated_info.IsInSector) {
    if (calculated_info.WaypointDistance<AATCloseDistance()*3.0) {
      LegBearing = AATCloseBearing();
    }
  }

  // JMW TODO accuracy: use mc based on risk? no!
  double LegAltitude =
    GlidePolar::MacCreadyAltitude(this_maccready,
                                  LegToGo,
                                  LegBearing,
                                  calculated_info.WindSpeed,
                                  calculated_info.WindBearing,
                                  &(calculated_info.BestCruiseTrack),
                                  &(calculated_info.VMacCready),

				  // (calculated_info.FinalGlide==1),
				  true,  // JMW CHECK FGAMT

                                  &(calculated_info.LegTimeToGo),
                                  height_above_finish, cruise_efficiency);

  double LegAltitude0 =
    GlidePolar::MacCreadyAltitude(0,
                                  LegToGo,
                                  LegBearing,
                                  calculated_info.WindSpeed,
                                  calculated_info.WindBearing,
                                  0,
                                  0,
                                  true,
                                  &LegTime0, 1.0e6, cruise_efficiency
                                  );

  if (calculated_info.IsInSector && (ActiveWayPoint==0) && !TaskIsTemporary()) {
    // set best cruise track to first leg bearing when in start sector
    calculated_info.BestCruiseTrack = StartBestCruiseTrack;
  }

  // JMW TODO accuracy: Use safetymc where appropriate

  LDNext(LegToGo);

  if (LegTime0>= 0.9*ERROR_TIME) {
    // can't make it, so assume flying at current mc
    LegAltitude0 = LegAltitude;
  }

  TaskAltitudeRequired += LegAltitude;
  TaskAltitudeRequired0 += LegAltitude0;
  calculated_info.TaskDistanceToGo += LegToGo;
  calculated_info.TaskTimeToGo += calculated_info.LegTimeToGo;

  height_above_finish-= LegAltitude;

  ////////////////

  if (calc_turning_now) {
    calculated_info.TaskTimeToGoTurningNow +=
      gps_info.Time-calculated_info.TaskStartTime;
  } else {
    calculated_info.TaskTimeToGoTurningNow = -1;
  }

  double final_height = FAIFinishHeight(-1);

  double total_energy_height = calculated_info.NavAltitude
    + calculated_info.EnergyHeight;

  calculated_info.TaskAltitudeRequired = TaskAltitudeRequired + final_height;

  TaskAltitudeRequired0 += final_height;

  calculated_info.TaskAltitudeDifference = total_energy_height
    - calculated_info.TaskAltitudeRequired;

  calculated_info.TaskAltitudeDifference0 = total_energy_height
    - TaskAltitudeRequired0;

  // VENTA6
  calculated_info.NextAltitudeDifference0 = total_energy_height
    - calculated_info.NextAltitudeRequired0;

  calculated_info.LDFinish = UpdateLD(calculated_info.LDFinish,
                                  calculated_info.TaskDistanceToGo,
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
  double GRsafecalc = calculated_info.NavAltitude - final_height;
  if (GRsafecalc <=0) calculated_info.GRFinish = INVALID_GR;
  else {
    calculated_info.GRFinish = calculated_info.TaskDistanceToGo / GRsafecalc;
    if ( calculated_info.GRFinish >ALTERNATE_MAXVALIDGR || calculated_info.GRFinish <0 ) calculated_info.GRFinish = INVALID_GR;
    else
      if ( calculated_info.GRFinish <1 ) calculated_info.GRFinish = 1;
  }
  // END VENTA-ADDON

  CheckFinalGlideThroughTerrain(LegToGo, LegBearing);

  CheckForceFinalGlide();

  mutexTaskData.Unlock();

}


void GlideComputerTask::AATStats_Time() {
  // Task time to go calculations

  double aat_tasktime_elapsed = gps_info.Time - calculated_info.TaskStartTime;
  double aat_tasklength_seconds = AATTaskLength*60;

  if (ActiveWayPoint==0) {
    if (calculated_info.AATTimeToGo==0) {
      calculated_info.AATTimeToGo = aat_tasklength_seconds;
    }
  } else if (aat_tasktime_elapsed>=0) {
    calculated_info.AATTimeToGo = max(0,
				  aat_tasklength_seconds
				  - aat_tasktime_elapsed);
  }

  if(ValidTaskPoint(ActiveWayPoint) && (calculated_info.AATTimeToGo>0)) {
    calculated_info.AATMaxSpeed =
      calculated_info.AATMaxDistance / calculated_info.AATTimeToGo;
    calculated_info.AATMinSpeed =
      calculated_info.AATMinDistance / calculated_info.AATTimeToGo;
    calculated_info.AATTargetSpeed =
      calculated_info.AATTargetDistance / calculated_info.AATTimeToGo;
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
        DistanceBearing(gps_info.Latitude , gps_info.Longitude ,
                        WayPointList[Task[i].Index].Latitude,
                        WayPointList[Task[i].Index].Longitude,
                        &LegToGo, NULL);

        DistanceBearing(gps_info.Latitude , gps_info.Longitude ,
                        Task[i].AATTargetLat,
                        Task[i].AATTargetLon,
                        &TargetLegToGo, NULL);

        if(Task[i].AATType == CIRCLE)
        {
          MaxDistance = LegToGo + (Task[i].AATCircleRadius );  // ToDo: should be adjusted for angle of max target and for national rules
          MinDistance = LegToGo - (Task[i].AATCircleRadius );
        }
        else
        {
          MaxDistance = LegToGo + (Task[i].AATSectorRadius );  // ToDo: should be adjusted for angle of max target.
          MinDistance = LegToGo;
        }

        TargetDistance = TargetLegToGo;
      }

      i++;
      while(ValidTaskPoint(i)) {
	double LegDistance, TargetLegDistance;

	DistanceBearing(WayPointList[Task[i].Index].Latitude,
			WayPointList[Task[i].Index].Longitude,
			WayPointList[Task[i-1].Index].Latitude,
			WayPointList[Task[i-1].Index].Longitude,
			&LegDistance, NULL);

	DistanceBearing(Task[i].AATTargetLat,
			Task[i].AATTargetLon,
			Task[i-1].AATTargetLat,
			Task[i-1].AATTargetLon,
			&TargetLegDistance, NULL);

	MaxDistance += LegDistance;
	MinDistance += LegDistance;

	if(Task[ActiveWayPoint].AATType == CIRCLE) {
	  // breaking out single Areas increases accuracy for start
	  // and finish

	  // sector at start of (i)th leg
	  if (i-1 == 0) {// first leg of task
	    // add nothing
	    MaxDistance -= StartRadius; // e.g. Sports 2009 US Rules A116.3.2.  To Do: This should be configured multiple countries
	    MinDistance -= StartRadius;
	  } else { // not first leg of task
	    MaxDistance += (Task[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	    MinDistance -= (Task[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }

	  // sector at end of ith leg
	  if (!ValidTaskPoint(i+1)) {// last leg of task
	    // add nothing
	    MaxDistance -= FinishRadius; // To Do: This can be configured for finish rules
	    MinDistance -= FinishRadius;
	  } else { // not last leg of task
	    MaxDistance += (Task[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	    MinDistance -= (Task[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }
	} else { // not circle (pie slice)
	  // sector at start of (i)th leg
	  if (i-1 == 0) {// first leg of task
	    // add nothing
	    MaxDistance += 0; // To Do: This can be configured for start rules
	  } else { // not first leg of task
	    MaxDistance += (Task[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }

	  // sector at end of ith leg
	  if (!ValidTaskPoint(i+1)) {// last leg of task
	    // add nothing
	    MaxDistance += 0; // To Do: This can be configured for finish rules
	  } else { // not last leg of task
	    MaxDistance += (Task[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }
	}
	TargetDistance += TargetLegDistance;
	i++;
      }

      // JMW TODO accuracy: make these calculations more accurate, because
      // currently they are very approximate.

      calculated_info.AATMaxDistance = MaxDistance;
      calculated_info.AATMinDistance = MinDistance;
      calculated_info.AATTargetDistance = TargetDistance;
    }
  mutexTaskData.Unlock();
}


void GlideComputerTask::AATStats()
{
  if (!WayPointList
      || !AATEnabled
      || calculated_info.ValidFinish) return ;

  AATStats_Distance();
  AATStats_Time();
}


void GlideComputerTask::CheckTransitionFinalGlide() {
  int FinalWayPoint = getFinalWaypoint();
  // update final glide mode status
  if (((ActiveWayPoint == FinalWayPoint)
       ||(ForceFinalGlide))
      && (ValidTaskPoint(ActiveWayPoint))) {

    if (calculated_info.FinalGlide == 0)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE);
    calculated_info.FinalGlide = 1;
  } else {
    if (calculated_info.FinalGlide == 1)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
    calculated_info.FinalGlide = 0;
  }

}


double GlideComputerTask::SpeedHeight() {
  if (calculated_info.TaskDistanceToGo<=0) {
    return 0;
  }

  // Fraction of task distance covered
  double d_fraction = calculated_info.TaskDistanceCovered/
    (calculated_info.TaskDistanceCovered+calculated_info.TaskDistanceToGo);

  double dh_start = calculated_info.TaskStartAltitude;

  double dh_finish = FAIFinishHeight(-1);

  // Excess height
  return calculated_info.NavAltitude
    - (dh_start*(1.0-d_fraction)+dh_finish*(d_fraction));
}


void GlideComputerTask::DebugTaskCalculations()
{
#ifdef DEBUGTASKSPEED
  if ((calculated_info.TaskStartTime>0)
      && (gps_info.Time-calculated_info.TaskStartTime>0)) {
      if (calculated_info.Flying) {

        double effective_mc = EffectiveMacCready();
        DebugStore("%g %g %g %g %g %g %g %g %g %g %d %g %g # taskspeed\r\n",
                gps_info.Time-calculated_info.TaskStartTime,
                calculated_info.TaskDistanceCovered,
                calculated_info.TaskDistanceToGo,
                calculated_info.TaskAltitudeRequired,
                calculated_info.NavAltitude,
                calculated_info.TaskSpeedAchieved,
                calculated_info.TaskSpeed,
                calculated_info.TaskSpeedInstantaneous,
                MACCREADY,
                effective_mc,
                ActiveWayPoint,
                calculated_info.DistanceVario,
                calculated_info.GPSVario);
      }
    }
#endif
}


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

  mutexTaskData.Lock();

  double height_above_finish = FAIFinishHeight( 0)-
    FAIFinishHeight( -1);

  for(i=MAXTASKPOINTS-2;i>=0;i--) {


    if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

    w1lat = WayPointList[Task[i].Index].Latitude;
    w1lon = WayPointList[Task[i].Index].Longitude;
    w0lat = WayPointList[Task[i+1].Index].Latitude;
    w0lon = WayPointList[Task[i+1].Index].Longitude;

    if (AATEnabled) {
      w1lat = Task[i].AATTargetLat;
      w1lon = Task[i].AATTargetLon;
      if (!isfinal) {
        w0lat = Task[i+1].AATTargetLat;
        w0lon = Task[i+1].AATTargetLon;
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
                                    calculated_info.WindSpeed,
                                    calculated_info.WindBearing,
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
      mutexTaskData.Unlock();
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
    calculated_info.TaskAltitudeRequiredFromStart = TotalAltitude;
    retval = false;
  } else {
    calculated_info.TaskAltitudeRequiredFromStart = TotalAltitude;
    retval = true;
  }
 OnExit:
  mutexTaskData.Unlock();
  return retval;
}


double GlideComputerTask::MacCreadyOrAvClimbRate(double this_maccready)
{
  double mc_val = this_maccready;
  bool is_final_glide = false;

  if (calculated_info.FinalGlide) {
    is_final_glide = true;
  }

  // when calculating 'achieved' task speed, need to use Mc if
  // not in final glide, or if in final glide mode and using
  // auto Mc, use the average climb rate achieved so far.

  if ((mc_val<0.1) ||
      (calculated_info.AutoMacCready &&
       ((AutoMcMode==0) ||
        ((AutoMcMode==2)&&(is_final_glide))
        ))
      ) {

    mc_val = GetAverageThermal();
  }
  return max(0.1, mc_val);

}


void GlideComputerTask::TaskSpeed(const double this_maccready,
				  const double cruise_efficiency)
{
  int ifinal;
  static double LastTime = 0;
  static double LastTimeStats = 0;
  double TotalTime=0, TotalDistance=0, Vfinal=0;

  if (!ValidTaskPoint(ActiveWayPoint)) return;
  if (TaskIsTemporary()) return;
  if (calculated_info.ValidFinish) return;
  if (!calculated_info.Flying) return;

  // in case we leave early due to error
  calculated_info.TaskSpeedAchieved = 0;
  calculated_info.TaskSpeed = 0;

  if (ActiveWayPoint<=0) { // no task speed before start
    calculated_info.TaskSpeedInstantaneous = 0;
    return;
  }

  mutexTaskData.Lock();

  if (TaskAltitudeRequired(this_maccready, &Vfinal,
                           &TotalTime, &TotalDistance, &ifinal, 
			   cruise_efficiency)) {

    double t0 = TotalTime;
    // total time expected for task

    double t1 = gps_info.Time-calculated_info.TaskStartTime;
    // time elapsed since start

    double d0 = TotalDistance;
    // total task distance

    double d1 = calculated_info.TaskDistanceCovered;
    // actual distance covered

    double dr = calculated_info.TaskDistanceToGo;
    // distance remaining

    double t2;
    // equivalent time elapsed after final glide

    double d2;
    // equivalent distance travelled after final glide

    double hf = FAIFinishHeight( -1);

    double h0 = calculated_info.TaskAltitudeRequiredFromStart-hf;
    // total height required from start (takes safety arrival alt
    // and finish waypoint altitude into account)

    double h1 = max(0,calculated_info.NavAltitude-hf);
    // height above target

    double dFinal;
    // final glide distance

    // equivalent speed
    double v2, v1;

    if ((t1<=0) || (d1<=0) || (d0<=0) || (t0<=0) || (h0<=0)) {
      // haven't started yet or not a real task
      calculated_info.TaskSpeedInstantaneous = 0;
      //?      calculated_info.TaskSpeed = 0;
      goto OnExit;
    }

    // JB's task speed...
    double hx = max(0,SpeedHeight());
    double t1mod = t1-hx/MacCreadyOrAvClimbRate(this_maccready);
    // only valid if flown for 5 minutes or more
    if (t1mod>300.0) {
      calculated_info.TaskSpeedAchieved = d1/t1mod;
    } else {
      calculated_info.TaskSpeedAchieved = d1/t1;
    }
    calculated_info.TaskSpeed = calculated_info.TaskSpeedAchieved;

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

    if (calculated_info.ValidFinish) {
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
    calculated_info.TaskSpeed = max(v1,v2);
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
    calculated_info.TaskSpeed = v2;
#endif

    double konst = 1.1;
    if (TaskModified)
      {
	konst = 1.0;
      }

    double termikLigaPoints = 0;
    if (d1 > 0)
      {
	termikLigaPoints = konst*(0.015*0.001*d1-(400.0/(0.001*d1))+12.0)*v1*3.6*100.0/(double)Handicap;
      }

    calculated_info.TermikLigaPoints = termikLigaPoints;

    if(gps_info.Time < LastTime) {
      LastTime = gps_info.Time;
    } else if (gps_info.Time-LastTime >=1.0) {

      double dt = gps_info.Time-LastTime;
      LastTime = gps_info.Time;

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

      static double dr_last = 0;

      double mc_safe = max(0.1,this_maccready);
      double Vstar = max(1.0,calculated_info.VMacCready);
      double vthis = (calculated_info.LegDistanceCovered-dr_last)/dt;
      vthis /= AirDensityRatio(calculated_info.NavAltitude);

      dr_last = calculated_info.LegDistanceCovered;
      double ttg = max(1,calculated_info.LegTimeToGo);
      //      double Vav = d0/max(1.0,t0);
      double Vrem = calculated_info.LegDistanceToGo/ttg;
      double Vref = // Vav;
	Vrem;
      double sr = -GlidePolar::SinkRate(Vstar);
      double height_diff = max(0,-calculated_info.TaskAltitudeDifference);

      if (calculated_info.timeCircling>30) {
	mc_safe = max(this_maccready,
		      calculated_info.TotalHeightClimb/calculated_info.timeCircling);
      }
      // circling percentage during cruise/climb
      double rho_cruise = max(0.0,min(1.0,mc_safe/(sr+mc_safe)));
      double rho_climb = 1.0-rho_cruise;
      double time_climb = height_diff/mc_safe;

      // calculate amount of time in cruise/climb glide
      double rho_c = max(0,min(1,time_climb/ttg));

      if (calculated_info.FinalGlide) {
	if (rho_climb>0) {
	  rho_c = max(0,min(1,rho_c/rho_climb));
	}
	if (!calculated_info.Circling) {
	  if (calculated_info.TaskAltitudeDifference>0) {
	    rho_climb *= rho_c;
	    rho_cruise *= rho_c;
	    // Vref = Vrem;
	  }
	}
      }

      double w_comp = min(10.0,max(-10.0,calculated_info.Vario/mc_safe));
      double vdiff = vthis/Vstar + w_comp*rho_cruise + rho_climb;

      if (vthis > SAFTEYSPEED*2) {
	vdiff = 1.0;
	// prevent funny numbers when starting mid-track
      }
      //      calculated_info.Experimental = vdiff*100.0;

      vdiff *= Vref;

      if (t1<5) {
        calculated_info.TaskSpeedInstantaneous = vdiff;
        // initialise
      } else {
        static int lastActiveWayPoint = 0;
	static double tsi_av = 0;
	static int n_av = 0;
        if ((ActiveWayPoint==lastActiveWayPoint)
	    && (calculated_info.LegDistanceToGo>1000.0)
	    && (calculated_info.LegDistanceCovered>1000.0)) {

          calculated_info.TaskSpeedInstantaneous =
            LowPassFilter(calculated_info.TaskSpeedInstantaneous, vdiff, 0.1);

          // update stats
          if(gps_info.Time < LastTimeStats) {
            LastTimeStats = gps_info.Time;
	    tsi_av = 0;
	    n_av = 0;
          } else if (n_av>=60) {
	    tsi_av/= n_av;

	    SaveTaskSpeed(max((gps_info.Time-calculated_info.TaskStartTime)/3600.0,
			      max(0,min(100.0,tsi_av))));

            LastTimeStats = gps_info.Time;
	    tsi_av = 0;
	    n_av = 0;
          }
	  tsi_av += calculated_info.TaskSpeedInstantaneous;
	  n_av ++;

        } else {

          calculated_info.TaskSpeedInstantaneous =
            LowPassFilter(calculated_info.TaskSpeedInstantaneous, vdiff, 0.5);

	  //	  calculated_info.TaskSpeedInstantaneous = vdiff;
	  tsi_av = 0;
	  n_av = 0;
	}
        lastActiveWayPoint = ActiveWayPoint;
      }
    }
  }
 OnExit:
  mutexTaskData.Unlock();

}


