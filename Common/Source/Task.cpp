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

#include "Task.h"
#include "TaskFile.hpp"
#include "Protection.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "SettingsTask.hpp"
#include "Waypointparser.h"
#include "McReady.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Units.hpp"
#include <math.h>
#include "Logger.h"
#include "Interface.hpp"
#include "Components.hpp"
#include "WayPointList.hpp"

#include <stdio.h>

static int Task_saved[MAXTASKPOINTS+1];
static int active_waypoint_saved= -1;
static bool aat_enabled_saved= false;

static bool TaskModified=false;
static bool TargetModified=false;
static bool TaskAborted = false;

static void BackupTask(void);

bool isTaskAborted() {
  return TaskAborted;
}

bool isTaskModified() {
  return TaskModified;
}

void SetTaskModified(const bool set) {
  TaskModified = set;
}

bool isTargetModified() {
  return TargetModified;
}

void SetTargetModified(const bool set) {
  TargetModified = set;
  if (set) {
    SetTaskModified();
  }
}

void ResetTaskWaypoint(int j) {
  task_points[j].Index = -1;
  task_stats[j].AATTargetOffsetRadius = 0.0;
  task_stats[j].AATTargetOffsetRadial = 0.0;
  task_stats[j].AATTargetLocked = false;
  task_points[j].AATSectorRadius = SectorRadius;
  task_points[j].AATCircleRadius = SectorRadius;
  task_points[j].AATStartRadial = 0;
  task_points[j].AATFinishRadial = 360;
}


void FlyDirectTo(int index, const SETTINGS_COMPUTER &settings_computer) {
  if (!CheckDeclaration())
    return;

  mutexTaskData.Lock();

  if (TaskAborted) {
    // in case we GOTO while already aborted
    ResumeAbortTask(settings_computer, -1);
  }

  if (!TaskIsTemporary()) {
    BackupTask();
  }

  TaskModified = true;
  TargetModified = true;
  ActiveTaskPoint = -1;

  AATEnabled = FALSE;

  /*  JMW disabled this so task info is preserved
  for(int j=0;j<MAXTASKPOINTS;j++)
  {
    ResetTaskWaypoint(j);
  }
  */

  task_points[0].Index = index;
  for (int i=1; i<=MAXTASKPOINTS; i++) {
    task_points[i].Index = -1;
  }
  ActiveTaskPoint = 0;
  RefreshTask(settings_computer);
  mutexTaskData.Unlock();
}


// Swaps waypoint at current index with next one.
void SwapWaypoint(int index,
		  const SETTINGS_COMPUTER &settings_computer) {
  if (!CheckDeclaration())
    return;

  mutexTaskData.Lock();
  TaskModified = true;
  TargetModified = true;
  if (index<0) {
    return;
  }
  if (index+1>= MAXTASKPOINTS-1) {
    return;
  }
  if ((task_points[index].Index != -1)&&(task_points[index+1].Index != -1)) {
    TASK_POINT tmpPoint;
    tmpPoint = task_points[index];
    task_points[index] = task_points[index+1];
    task_points[index+1] = tmpPoint;
  }
  RefreshTask(settings_computer);
  mutexTaskData.Unlock();
}


// Inserts a waypoint into the task, in the
// position of the ActiveWaypoint.  If append=true, insert at end of the
// task.
void InsertWaypoint(int index, const SETTINGS_COMPUTER &settings_computer,
		    bool append) {
  if (!CheckDeclaration())
    return;

  int i;
  ScopeLock protect(mutexTaskData);
  TaskModified = true;
  TargetModified = true;

  if ((ActiveTaskPoint<0) || !ValidTaskPoint(0)) {
    ActiveTaskPoint = 0;
    ResetTaskWaypoint(ActiveTaskPoint);
    task_points[ActiveTaskPoint].Index = index;
    RefreshTask(settings_computer);
    return;
  }

  if (ValidTaskPoint(MAXTASKPOINTS-1)) {
    // No room for any more task points!
    MessageBoxX(
      gettext(TEXT("Too many waypoints in task!")),
      gettext(TEXT("Insert Waypoint")),
      MB_OK|MB_ICONEXCLAMATION);
    return;
  }

  int indexInsert = max(ActiveTaskPoint,0);
  if (append) {
    for (i=indexInsert; i<MAXTASKPOINTS-2; i++) {
      if (task_points[i+1].Index<0) {
	ResetTaskWaypoint(i+1);
	task_points[i+1].Index = index;
	break;
      }
    }
  } else {
    // Shuffle ActiveWaypoint and all later task points
    // to the right by one position
    for (i=MAXTASKPOINTS-1; i>indexInsert; i--) {
      task_points[i] = task_points[i-1];
    }
    // Insert new point and update task details
    ResetTaskWaypoint(indexInsert);
    task_points[indexInsert].Index = index;
  }

  RefreshTask(settings_computer);
}

// Create a default task to home at startup if no task is present
void DefaultTask(const SETTINGS_COMPUTER &settings_computer) {
  mutexTaskData.Lock();
  TaskModified = true;
  TargetModified = true;
  if ((task_points[0].Index == -1)||(ActiveTaskPoint==-1)) {
    if (settings_computer.HomeWaypoint != -1) {
      task_points[0].Index = settings_computer.HomeWaypoint;
      ActiveTaskPoint = 0;
    }
  }
  RefreshTask(settings_computer);
  mutexTaskData.Unlock();
}


// RemoveTaskpoint removes a single waypoint
// from the current task.  index specifies an entry
// in the task_points[] array - NOT a waypoint index.
//
// If you call this function, you MUST deal with
// correctly setting ActiveTaskPoint yourself!
void RemoveTaskPoint(int index, 
		     const SETTINGS_COMPUTER &settings_computer) {
  if (!CheckDeclaration())
    return;

  int i;

  if (index < 0 || index >= MAXTASKPOINTS) {
    return; // index out of bounds
  }

  mutexTaskData.Lock();
  TaskModified = true;
  TargetModified = true;

  if (task_points[index].Index == -1) {
    mutexTaskData.Unlock();
    return; // There's no WP at this location
  }

  // Shuffle all later taskpoints to the left to
  // fill the gap
  for (i=index; i<MAXTASKPOINTS-1; ++i) {
    task_points[i] = task_points[i+1];
  }
  task_points[MAXTASKPOINTS-1].Index = -1;
  task_stats[MAXTASKPOINTS-1].AATTargetOffsetRadius= 0.0;

  RefreshTask(settings_computer);
  mutexTaskData.Unlock();

}


// Index specifies a waypoint in the WP list
// It won't necessarily be a waypoint that's
// in the task
void RemoveWaypoint(int index,
		    const SETTINGS_COMPUTER &settings_computer) {
  int i;

  if (!CheckDeclaration())
    return;

  if (ActiveTaskPoint<0) {
    return; // No waypoint to remove
  }

  // Check to see whether selected WP is actually
  // in the task list.
  // If not, we'll ask the user if they want to remove
  // the currently active task point.
  // If the WP is in the task multiple times then we'll
  // remove the first instance after (or including) the
  // active WP.
  // If they're all before the active WP then just remove
  // the nearest to the active WP

  mutexTaskData.Lock();
  TaskModified = true;
  TargetModified = true;

  // Search forward first
  i = ActiveTaskPoint;
  while ((i < MAXTASKPOINTS) && (task_points[i].Index != index)) {
    ++i;
  }

  if (i < MAXTASKPOINTS) {
    // Found WP, so remove it
    RemoveTaskPoint(i, settings_computer);

    if (task_points[ActiveTaskPoint].Index == -1) {
      // We've just removed the last task point and it was
      // active at the time
      ActiveTaskPoint--;
    }

  } else {
    // Didn't find WP, so search backwards

    i = ActiveTaskPoint;
    do {
      --i;
    } while (i >= 0 && task_points[i].Index != index);

    if (i >= 0) {
      // Found WP, so remove it
      RemoveTaskPoint(i, settings_computer);
      ActiveTaskPoint--;

    } else {
      // WP not found, so ask user if they want to
      // remove the active WP
      mutexTaskData.Unlock();
      int ret = MessageBoxX(
        gettext(TEXT("Chosen Waypoint not in current task.\nRemove active WayPoint?")),
        gettext(TEXT("Remove Waypoint")),
        MB_YESNO|MB_ICONQUESTION);
      mutexTaskData.Lock();

      if (ret == IDYES) {
        RemoveTaskPoint(ActiveTaskPoint, settings_computer);
        if (task_points[ActiveTaskPoint].Index == -1) {
          // Active WayPoint was last in the list so is currently
          // invalid.
          ActiveTaskPoint--;
        }
      }
    }
  }
  RefreshTask(settings_computer);
  mutexTaskData.Unlock();

}


void ReplaceWaypoint(int index,
		     const SETTINGS_COMPUTER &settings_computer) {
  if (!CheckDeclaration())
    return;

  mutexTaskData.Lock();
  TaskModified = true;
  TargetModified = true;

  // ARH 26/06/05 Fixed array out-of-bounds bug
  if (ActiveTaskPoint>=0) {
    ResetTaskWaypoint(ActiveTaskPoint);
    task_points[ActiveTaskPoint].Index = index;
  } else {

    // Insert a new waypoint since there's
    // nothing to replace
    ActiveTaskPoint=0;
    ResetTaskWaypoint(ActiveTaskPoint);
    task_points[ActiveTaskPoint].Index = index;
  }
  RefreshTask(settings_computer);
  mutexTaskData.Unlock();
}

static void CalculateAATTaskSectors(const NMEA_INFO &gps_info);

extern void RefreshTask_Visitor(const SETTINGS_COMPUTER &settings_computer);


void RefreshTask(const SETTINGS_COMPUTER &settings_computer) {
  ScopeLock protect(mutexTaskData);
  RefreshTask_Visitor(settings_computer);
  CalculateAATTaskSectors(XCSoarInterface::Basic());
}



void RotateStartPoints(const SETTINGS_COMPUTER &settings_computer) {
  if (ActiveTaskPoint>0) return;
  if (!EnableMultipleStartPoints) return;

  mutexTaskData.Lock();

  int found = -1;
  int imax = 0;
  for (int i=0; i<MAXSTARTPOINTS; i++) {
    if (task_start_stats[i].Active 
        && way_points.verify_index(task_start_points[i].Index)) {
      if (task_points[0].Index == task_start_points[i].Index) {
        found = i;
      }
      imax = i;
    }
  }
  found++;
  if (found>imax) {
    found = 0;
  }
  if (way_points.verify_index(task_start_points[found].Index)) {
    task_points[0].Index = task_start_points[found].Index;
  }

  RefreshTask(settings_computer);
  mutexTaskData.Unlock();
}


double AdjustAATTargets(double desired) {
  int i, istart, inum;
  double av=0;
  istart = max(1,ActiveTaskPoint);
  inum=0;

  mutexTaskData.Lock();
  for(i=istart;i<MAXTASKPOINTS-1;i++)
    {
      if(ValidTaskPoint(i)&&ValidTaskPoint(i+1) && !task_stats[i].AATTargetLocked)
	{
          task_stats[i].AATTargetOffsetRadius = max(-1,min(1,
                                          task_stats[i].AATTargetOffsetRadius));
	  av += task_stats[i].AATTargetOffsetRadius;
	  inum++;
	}
    }
  if (inum>0) {
    av/= inum;
  }
  if (fabs(desired)>1.0) {
    // don't adjust, just retrieve.
    goto OnExit;
  }

  // TODO accuracy: Check here for true minimum distance between
  // successive points (especially second last to final point)

  // Do this with intersection tests

  desired = (desired+1.0)/2.0; // scale to 0,1
  av = (av+1.0)/2.0; // scale to 0,1

  for(i=istart;i<MAXTASKPOINTS-1;i++)
    {
      if((task_points[i].Index >=0)
	 &&(task_points[i+1].Index >=0) 
	 && !task_stats[i].AATTargetLocked)
	{
	  double d = (task_stats[i].AATTargetOffsetRadius+1.0)/2.0;
          // scale to 0,1

          if (av>0.01) {
            d = desired;
	    // 20080615 JMW
	    // was (desired/av)*d;
	    // now, we don't want it to be proportional
          } else {
            d = desired;
          }
          d = min(1.0, max(d, 0))*2.0-1.0;
          task_stats[i].AATTargetOffsetRadius = d;
	}
    }
 OnExit:
  mutexTaskData.Unlock();
  return av;
}

static void
CalculateAATTaskSectors(const NMEA_INFO &gps_info)
{
  int i;
  int awp = ActiveTaskPoint;

  if(AATEnabled == FALSE)
    return;

  mutexTaskData.Lock();

  task_stats[0].AATTargetOffsetRadius = 0.0;
  task_stats[0].AATTargetOffsetRadial = 0.0;
  if (task_points[0].Index>=0) {
    task_stats[0].AATTargetLocation = way_points.get(task_points[0].Index).Location;
  }

  for(i=1;i<MAXTASKPOINTS;i++) {
    if(ValidTaskPoint(i)) {
      if (!ValidTaskPoint(i+1)) {
        // This must be the final waypoint, so it's not an AAT OZ
        task_stats[i].AATTargetLocation = way_points.get(task_points[i].Index).Location;
        continue;
      }

      if(task_points[i].AATType == SECTOR) {
        FindLatitudeLongitude (way_points.get(task_points[i].Index).Location,
                               task_points[i].AATStartRadial,
                               task_points[i].AATSectorRadius,
                               &task_points[i].AATStart);

        FindLatitudeLongitude (way_points.get(task_points[i].Index).Location,
                               task_points[i].AATFinishRadial ,
                               task_points[i].AATSectorRadius,
                               &task_points[i].AATFinish);
      }

      // JMWAAT: if locked, don't move it
      if (i<awp) {
        // only update targets for current/later waypoints
        continue;
      }

      task_stats[i].AATTargetOffsetRadius =
        min(1.0, max(task_stats[i].AATTargetOffsetRadius,-1.0));

      task_stats[i].AATTargetOffsetRadial =
        min(90, max(-90, task_stats[i].AATTargetOffsetRadial));

      double targetbearing;
      double targetrange;

      targetbearing = AngleLimit360(task_points[i].Bisector+task_stats[i].AATTargetOffsetRadial);

      if(task_points[i].AATType == SECTOR) {

        //AATStartRadial
        //AATFinishRadial

        targetrange = ((task_stats[i].AATTargetOffsetRadius+1.0)/2.0);

        double aatbisector = HalfAngle(task_points[i].AATStartRadial,
                                       task_points[i].AATFinishRadial);

        if (fabs(AngleLimit180(aatbisector-targetbearing))>90) {
          // bisector is going away from sector
          targetbearing = Reciprocal(targetbearing);
          targetrange = 1.0-targetrange;
        }
        if (!AngleInRange(task_points[i].AATStartRadial,
                          task_points[i].AATFinishRadial,
                          targetbearing,true)) {

          // Bisector is not within AAT sector, so
          // choose the closest radial as the target line

          if (fabs(AngleLimit180(task_points[i].AATStartRadial-targetbearing))
              <fabs(AngleLimit180(task_points[i].AATFinishRadial-targetbearing))) {
            targetbearing = task_points[i].AATStartRadial;
          } else {
            targetbearing = task_points[i].AATFinishRadial;
          }
        }

        targetrange*= task_points[i].AATSectorRadius;

      } else {
        targetrange = task_stats[i].AATTargetOffsetRadius
          *task_points[i].AATCircleRadius;
      }

      // TODO accuracy: if i=awp and in sector, range parameter needs to
      // go from current aircraft position to projection of target
      // out to the edge of the sector

      if (InAATTurnSector(gps_info.Location, i) && (awp==i) &&
          !task_stats[i].AATTargetLocked) {

        // special case, currently in AAT sector/cylinder

        double dist;
        double qdist;
        double bearing;

        // find bearing from last target through current aircraft position with offset
        DistanceBearing(task_stats[i-1].AATTargetLocation,
                        gps_info.Location,
                        &qdist, &bearing);

        bearing = AngleLimit360(bearing+task_stats[i].AATTargetOffsetRadial);

        dist = ((task_stats[i].AATTargetOffsetRadius+1)/2.0)*
          FindInsideAATSectorDistance(gps_info.Location, i, bearing);

        // if (dist+qdist>aatdistance.LegDistanceAchieved(awp)) {
        // JMW: don't prevent target from being closer to the aircraft
        // than the best achieved, so can properly plan arrival time

        FindLatitudeLongitude (gps_info.Location,
                               bearing,
                               dist,
                               &task_stats[i].AATTargetLocation);

        TargetModified = true;

        // }

      } else {

        FindLatitudeLongitude (way_points.get(task_points[i].Index).Location,
                               targetbearing,
                               targetrange,
                               &task_stats[i].AATTargetLocation);
        TargetModified = true;

      }
    }
  }

  CalculateAATIsoLines();
  if (!targetManipEvent.test()) {
    TargetModified = false;
    // allow target dialog to detect externally changed targets
  }

  mutexTaskData.Unlock();
}


void ClearTask(void) {
  mutexTaskData.Lock();

  memset( &(task_points), 0, sizeof(Task_t));
  memset( &(task_start_points), 0, sizeof(Start_t));

  TaskModified = true;
  TargetModified = true;
  ClearTaskFileName();
  ActiveTaskPoint = -1;
  int i;
  for(i=0;i<MAXTASKPOINTS;i++) {
    task_points[i].Index = -1;
    task_points[i].AATSectorRadius = SectorRadius; // JMW added default
    task_points[i].AATCircleRadius = SectorRadius; // JMW added default
    task_stats[i].AATTargetOffsetRadial = 0;
    task_stats[i].AATTargetOffsetRadius = 0;
    task_stats[i].AATTargetLocked = false;
    for (int j=0; j<MAXISOLINES; j++) {
      task_stats[i].IsoLine_valid[j] = false;
    }
    Task_saved[i] = task_points[i].Index;
  }
  for (i=0; i<MAXSTARTPOINTS; i++) {
    task_start_points[i].Index = -1;
  }
  mutexTaskData.Unlock();
}


bool ValidTask()  {
  return ValidTaskPoint(ActiveTaskPoint);
}

bool ValidTaskPoint(const int i) {
  ScopeLock protect(mutexTaskData);
  if ((i<0) || (i>= MAXTASKPOINTS))
    return false;
  else if (!way_points.verify_index(task_points[i].Index))
    return false;
  else 
    return true;
}


double FindInsideAATSectorDistance(const GEOPOINT &location,
                                   const int taskwaypoint,
                                   const double course_bearing,
                                   const double p_found) {

  double max_distance;
  if(task_points[taskwaypoint].AATType == SECTOR) {
    max_distance = task_points[taskwaypoint].AATSectorRadius;
  } else {
    max_distance = task_points[taskwaypoint].AATCircleRadius;
  }

  // Do binary bounds search for longest distance within sector

  double delta = max_distance;
  double t_distance_lower = p_found;
  double t_distance = p_found+delta*2;
  int steps = 0;
  do {

    GEOPOINT t_loc;
    FindLatitudeLongitude(location,
                          course_bearing, t_distance,
                          &t_loc);

    if (InAATTurnSector(t_loc, taskwaypoint)) {
      t_distance_lower = t_distance;
      // ok, can go further
      t_distance += delta;
    } else {
      t_distance -= delta;
    }
    delta /= 2.0;
  } while ((delta>5.0)&&(steps++<20));

  return t_distance_lower;
}


double FindInsideAATSectorRange(const GEOPOINT &location,
                                const int taskwaypoint,
                                const double course_bearing,
                                const double p_found) {

  double t_distance = FindInsideAATSectorDistance(location, taskwaypoint,
                                                  course_bearing, p_found);
  return (p_found /
          max(1,t_distance))*2-1;
}


/////////////////

double DoubleLegDistance(const int taskwaypoint,
                         const GEOPOINT &location) {
  if (taskwaypoint>0) {
    return DoubleDistance(task_stats[taskwaypoint-1].AATTargetLocation,
			  location,
			  task_stats[taskwaypoint+1].AATTargetLocation);
  } else {
    return Distance(location,
		    task_stats[taskwaypoint+1].AATTargetLocation);
  }
}

//////////////////////////////////////////////////////



bool TaskIsTemporary(void) {
  bool retval = false;
  mutexTaskData.Lock();
  if (TaskAborted) {
    retval = true;
  }
  if ((task_points[0].Index>=0) && (task_points[1].Index== -1)
      && (Task_saved[0] >= 0)) {
    retval = true;
  };

  mutexTaskData.Unlock();
  return retval;
}


static void BackupTask(void) {
  mutexTaskData.Lock();
  for (int i=0; i<=MAXTASKPOINTS; i++) {
    Task_saved[i]= task_points[i].Index;
  }
  active_waypoint_saved = ActiveTaskPoint;
  if (AATEnabled) {
    aat_enabled_saved = true;
  } else {
    aat_enabled_saved = false;
  }
  mutexTaskData.Unlock();
}


void ResumeAbortTask(const SETTINGS_COMPUTER &settings_computer, int set) {
  int i;
  int active_waypoint_on_entry;
  bool task_temporary_on_entry = TaskIsTemporary();

  mutexTaskData.Lock();
  active_waypoint_on_entry = ActiveTaskPoint;

  if (set == 0) {
    if (task_temporary_on_entry && !TaskAborted) {
      // no toggle required, we are resuming a temporary goto
    } else {
      TaskAborted = !TaskAborted;
    }
  } else if (set > 0)
    TaskAborted = true;
  else if (set < 0)
    TaskAborted = false;

  if (task_temporary_on_entry != TaskAborted) {
    if (TaskAborted) {

      // save current task in backup
      BackupTask();

      // force new waypoint to be the closest
      ActiveTaskPoint = -1;

      // force AAT off
      AATEnabled = false;

      // set MacCready
      if (!GlidePolar::AbortSafetyUseCurrent)  // 20060520:sgi added
	GlidePolar::SetMacCready(min(GlidePolar::GetMacCready(), 
				     GlidePolar::AbortSafetyMacCready()));

    } else {

      // reload backup task and clear it

      for (i=0; i<=MAXTASKPOINTS; i++) {
        task_points[i].Index = Task_saved[i];
	Task_saved[i] = -1;
      }
      ActiveTaskPoint = active_waypoint_saved;
      AATEnabled = aat_enabled_saved;

      RefreshTask(settings_computer);
    }
  }

  if (active_waypoint_on_entry != ActiveTaskPoint){
    SelectedWaypoint = ActiveTaskPoint;
  }

  mutexTaskData.Unlock();
}



int getFinalWaypoint() {
  int i;
  i=max(-1,min(MAXTASKPOINTS,ActiveTaskPoint));
  if (TaskAborted) {
    return i;
  }

  i++;
  mutexTaskData.Lock();
  while((i<MAXTASKPOINTS) && (task_points[i].Index != -1)) {
    i++;
  }
  mutexTaskData.Unlock();
  return i-1;
}

bool ActiveIsFinalWaypoint() {
  return (ActiveTaskPoint == getFinalWaypoint());
}


bool IsFinalWaypoint(void) {
  bool retval;
  mutexTaskData.Lock();
  if (ValidTask() && (task_points[ActiveTaskPoint+1].Index >= 0)) {
    retval = false;
  } else {
    retval = true;
  }
  mutexTaskData.Unlock();
  return retval;
}


bool InAATTurnSector(const GEOPOINT &location,
                    const int the_turnpoint)
{
  double AircraftBearing;
  bool retval = false;

  if (!ValidTaskPoint(the_turnpoint)) {
    return false;
  }

  double distance;
  mutexTaskData.Lock();
  DistanceBearing(way_points.get(task_points[the_turnpoint].Index).Location,
                  location, &distance, &AircraftBearing);

  if(task_points[the_turnpoint].AATType ==  CIRCLE) {
    if(distance < task_points[the_turnpoint].AATCircleRadius) {
      retval = true;
    }
  } else if(distance < task_points[the_turnpoint].AATSectorRadius) {
    if (AngleInRange(task_points[the_turnpoint].AATStartRadial,
                     task_points[the_turnpoint].AATFinishRadial,
                     AngleLimit360(AircraftBearing), true)) {
      retval = true;
    }
  }

  mutexTaskData.Unlock();
  return retval;
}


void CheckStartPointInTask(void) {
  mutexTaskData.Lock();
  if (task_points[0].Index != -1) {
    // ensure current start point is in task
    int index_last = 0;
    for (int i=MAXSTARTPOINTS-1; i>=0; i--) {
      if (task_start_points[i].Index == task_points[0].Index) {
	index_last = -1;
	break;
      }
      if ((task_start_points[i].Index>=0) && (index_last==0)) {
	index_last = i;
      }
    }
    if (index_last>=0) {
      if (task_start_points[index_last].Index>= 0) {
	index_last = min(MAXSTARTPOINTS-1,index_last+1);
      }
      // it wasn't, so make sure it's added now
      task_start_points[index_last].Index = task_points[0].Index;
      task_start_stats[index_last].Active = true;
    }
  }
  mutexTaskData.Unlock();
}


void ClearStartPoints()
{
  mutexTaskData.Lock();
  for (int i=0; i<MAXSTARTPOINTS; i++) {
    task_start_points[i].Index = -1;
    task_start_stats[i].Active = false;
  }
  task_start_points[0].Index = task_points[0].Index;
  task_start_stats[0].Active = true;
  mutexTaskData.Unlock();
}

void SetStartPoint(const int pointnum, const int waypointnum)
{
  if ((pointnum>=0) && (pointnum<MAXSTARTPOINTS)) {
    // TODO bug: don't add it if it's already present!
    mutexTaskData.Lock();
    task_start_points[pointnum].Index = waypointnum;
    task_start_stats[pointnum].Active = true;
    mutexTaskData.Unlock();
  }
}

