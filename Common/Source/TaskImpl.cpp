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

#include "TaskImpl.hpp"
#include "TaskFile.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "SettingsTask.hpp"
#include "WayPointParser.h"
#include "MacCready.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Units.hpp"
#include <math.h>
#include "Logger.h"
#include "Components.hpp"
#include "WayPointList.hpp"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "NMEA/Info.h"

#include <stdio.h>
#include <assert.h>

// TODO: separate out all waypoint checking, this should be done
// outside scope of task lock

static int Task_saved[MAXTASKPOINTS+1];

/**
 * Constructor of the Task class
 * @return
 */
Task::Task():
  ActiveTaskPoint(0),
  active_waypoint_saved(0),
  aat_enabled_saved(false),
  TaskModified(false),
  TargetModified(false),
  TaskAborted (false),
  AdvanceArmed (false),
  SelectedWaypoint(-1)
{
  settings.AATEnabled = false;
  settings.AutoAdvance = AUTOADVANCE_AUTO;
  settings.EnableMultipleStartPoints = false;
  settings.EnableFAIFinishHeight = false;
  settings.FinishType= FINISH_LINE;
  settings.FinishRadius=1000;
  settings.SectorType = AST_FAI;
  settings.SectorRadius = 10000;
  settings.StartType = START_LINE;
  settings.StartRadius = 3000;
  settings.AATTaskLength = 120;
  settings.FinishMinHeight = 0;
  settings.StartHeightRef = 0; // MSL
  settings.StartMaxHeight = 0;
  settings.StartMaxSpeed = 0;
  settings.StartMaxHeightMargin = 0;
  settings.StartMaxSpeedMargin = 0;

  ClearTask();
  ClearStartPoints();
}

/**
 * Returns whether AutoAdvance is armed
 * @return True if AutoAdvance is armed, False otherwise
 */
bool Task::isAdvanceArmed() const {
  return AdvanceArmed;
}

/**
 * Sets the AutoAdvance arm status
 * @param val The new arm status
 */
void Task::setAdvanceArmed(const bool val) {
  AdvanceArmed = val;
}

/**
 * Returns whether the task is aborted
 * @return True if the task is aborted, False otherwise
 */
bool Task::isTaskAborted() const {
  return TaskAborted;
}

/**
 * Returns whether the task is modified
 * @return True if the task is modified, False otherwise
 */
bool Task::isTaskModified() const {
  return TaskModified;
}

/**
 * Sets whether the task is modified or not
 * @param set True if the task is modified, False otherwise
 */
void Task::SetTaskModified(const bool set) {
  TaskModified = set;
}

/**
 * Returns whether the target is modified
 * @return True if the target is modified, False otherwise
 */
bool
Task::isTargetModified() const {
  return TargetModified;
}

/**
 * Sets whether the target is modified or not
 * and calls SetTaskModified() if True
 * @param set True if the target is modified, False otherwise
 */
void Task::SetTargetModified(const bool set) {
  TargetModified = set;
  if (set) {
    SetTaskModified();
  }
}

/**
 * Resets the Waypoint in task_points defined by j
 * @param j
 */
void Task::ResetTaskWaypoint(int j) {
  task_points[j].Index = -1;
  task_points[j].AATTargetOffsetRadius = 0.0;
  task_points[j].AATTargetOffsetRadial = 0.0;
  task_points[j].AATTargetLocked = false;
  task_points[j].AATSectorRadius = settings.SectorRadius;
  task_points[j].AATCircleRadius = settings.SectorRadius;
  task_points[j].AATStartRadial = 0;
  task_points[j].AATFinishRadial = 360;
}


void
Task::FlyDirectTo(const int index,
                  const SETTINGS_COMPUTER &settings_computer,
                  const NMEA_INFO &nmea_info)
{
  // if (a Task is declared to the Logger) leave function
  if (!logger.CheckDeclaration())
    return;

  if (TaskAborted) {
    // in case we GOTO while already aborted
    ResumeAbortTask(settings_computer, nmea_info, -1);
  }

  if (!TaskIsTemporary()) {
    BackupTask();
  }

  TaskModified = true;
  TargetModified = true;
  settings.AATEnabled = FALSE;

  /*  JMW disabled this so task info is preserved
  for(int j=0;j<MAXTASKPOINTS;j++)
  {
    ResetTaskWaypoint(j);
  }
  */

  ActiveTaskPoint = 0;
  task_points[0].Index = index;
  for (int i=1; i<=MAXTASKPOINTS; i++) {
    task_points[i].Index = -1;
  }

  RefreshTask(settings_computer, nmea_info);
}

/**
 * Swaps waypoint at given index with next one
 * @param index Index of the first waypoint to be swaped
 * @param settings_computer SettingsComputer object
 */
void
Task::SwapWaypoint(const int index,
                   const SETTINGS_COMPUTER &settings_computer,
                   const NMEA_INFO &nmea_info)
{
  if (!logger.CheckDeclaration())
    return;

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

  RefreshTask(settings_computer, nmea_info);
}

// Inserts a waypoint into the task, in the
// position of the ActiveWaypoint.  If append=true, insert at end of the
// task.
void
Task::InsertWaypoint(const int index,
                     const SETTINGS_COMPUTER &settings_computer,
                     const NMEA_INFO &nmea_info,
                     bool append)
{
  if (!logger.CheckDeclaration())
    return;

  int i;
  TaskModified = true;
  TargetModified = true;

  if (!ValidTaskPoint(0)) {
    ActiveTaskPoint = 0;
    ResetTaskWaypoint(ActiveTaskPoint);
    task_points[ActiveTaskPoint].Index = index;
    RefreshTask(settings_computer, nmea_info);
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

  int indexInsert = ActiveTaskPoint;
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

  RefreshTask(settings_computer, nmea_info);
}

// Create a default task to home at startup if no task is present
void
Task::DefaultTask(const SETTINGS_COMPUTER &settings_computer,
                  const NMEA_INFO &nmea_info)
{
  if (ValidTaskPoint(0))
    return;

  if (settings_computer.HomeWaypoint == -1)
    return;

  TaskModified = true;
  TargetModified = true;
  task_points[0].Index = settings_computer.HomeWaypoint;
  ActiveTaskPoint = 0;
  RefreshTask(settings_computer, nmea_info);
}

// RemoveTaskpoint removes a single waypoint
// from the current task.  index specifies an entry
// in the task_points[] array - NOT a waypoint index.
//
// If you call this function, you MUST deal with
// correctly setting ActiveTaskPoint yourself!
void
Task::RemoveTaskPoint(int index,
                      const SETTINGS_COMPUTER &settings_computer,
                      const NMEA_INFO &nmea_info)
{
  if (!logger.CheckDeclaration())
    return;

  int i;

  if (index < 0 || index >= MAXTASKPOINTS) {
    return; // index out of bounds
  }

  TaskModified = true;
  TargetModified = true;

  if (task_points[index].Index == -1) {
    return; // There's no WP at this location
  }

  // Shuffle all later taskpoints to the left to
  // fill the gap
  for (i=index; i<MAXTASKPOINTS-1; ++i) {
    task_points[i] = task_points[i+1];
  }
  task_points[MAXTASKPOINTS-1].Index = -1;
  task_points[MAXTASKPOINTS-1].AATTargetOffsetRadius= 0.0;

  if (ActiveTaskPoint>=(unsigned)index) {
    ActiveTaskPoint--;
  }

  RefreshTask(settings_computer, nmea_info);
}

// Index specifies a waypoint in the WP list
// It won't necessarily be a waypoint that's
// in the task
void
Task::RemoveWaypoint(const int index,
                     const SETTINGS_COMPUTER &settings_computer,
                     const NMEA_INFO &nmea_info)
{
  int i;

  if (!logger.CheckDeclaration())
    return;

  if (!ValidTaskPoint(0)) {
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

  TaskModified = true;
  TargetModified = true;

  // Search forward first
  i = ActiveTaskPoint;
  while ((i < MAXTASKPOINTS) && (task_points[i].Index != index)) {
    ++i;
  }

  if (i < MAXTASKPOINTS) {
    // Found WP, so remove it
    RemoveTaskPoint(i, settings_computer, nmea_info);

    if (task_points[ActiveTaskPoint].Index == -1) {
      // We've just removed the last task point and it was
      // active at the time
      if (ActiveTaskPoint) {
        ActiveTaskPoint--;
      }
    }

  } else {
    // Didn't find WP, so search backwards

    i = ActiveTaskPoint;
    do {
      --i;
    } while ((i >= 0) && (task_points[i].Index != index));

    if (i >= 0) {
      // Found WP, so remove it
      RemoveTaskPoint(i, settings_computer, nmea_info);
      if (ActiveTaskPoint) {
        ActiveTaskPoint--;
      }
    } else {
      // WP not found, so ask user if they want to
      // remove the active WP
      int ret = MessageBoxX(
        gettext(TEXT("Chosen Waypoint not in current task.\nRemove active WayPoint?")),
        gettext(TEXT("Remove Waypoint")),
        MB_YESNO|MB_ICONQUESTION);

      if (ret == IDYES) {
        RemoveTaskPoint(ActiveTaskPoint, settings_computer, nmea_info);
        if (task_points[ActiveTaskPoint].Index == -1) {
          // Active WayPoint was last in the list so is currently
          // invalid.
          if (ActiveTaskPoint) {
            ActiveTaskPoint--;
          }
        }
      }
    }
  }
  RefreshTask(settings_computer, nmea_info);
}

void
Task::ReplaceWaypoint(const int index,
                      const SETTINGS_COMPUTER &settings_computer,
                      const NMEA_INFO &nmea_info)
{
  if (!logger.CheckDeclaration())
    return;

  if (ValidTaskPoint(ActiveTaskPoint)) {
    TaskModified = true;
    TargetModified = true;
    ResetTaskWaypoint(ActiveTaskPoint);
    task_points[ActiveTaskPoint].Index = index;
  } else {
    // Insert a new waypoint since there's
    // nothing to replace
    TaskModified = true;
    TargetModified = true;
    ActiveTaskPoint=0;
    ResetTaskWaypoint(ActiveTaskPoint);
    task_points[ActiveTaskPoint].Index = index;
  }
  RefreshTask(settings_computer, nmea_info);
}

void
Task::RefreshTask(const SETTINGS_COMPUTER &settings_computer,
                  const NMEA_INFO &nmea_info)
{
  RefreshTask_Visitor(settings_computer);
  CalculateAATTaskSectors(nmea_info);
}

const GEOPOINT&
Task::getTargetLocation(const int v) const
{
  int r= (v==-1)? ActiveTaskPoint:v;
  if (settings.AATEnabled && (r>0) && !TaskIsTemporary()
      && ValidTaskPoint(r+1)) {
    return task_points[r].AATTargetLocation;
  } else {
    return getTaskPointLocation(r);
  }
}

const GEOPOINT&
Task::getActiveLocation() const
{
  return getTaskPointLocation(ActiveTaskPoint);
}

const GEOPOINT&
Task::getTaskPointLocation(const unsigned i) const
{
  assert(ValidTaskPoint(i));
  return way_points.get(task_points[i].Index).Location;
}

int
Task::getWaypointIndex(const int v) const
{
  int r= (v==-1)? ActiveTaskPoint:v;
  if (ValidTaskPoint(r)) {
    return task_points[r].Index;
  } else {
    return -1;
  }
}

WAYPOINT null_waypoint; // TODO

const WAYPOINT&
Task::getWaypoint(const int v) const
{
  int r= (v==-1)? ActiveTaskPoint:v;
  if (ValidTaskPoint(r)) {
    return way_points.get(task_points[r].Index);
  } else {
    return null_waypoint;
  }
}

const TASK_POINT&
Task::getTaskPoint(const int v) const
{
  int r= (v==-1)? ActiveTaskPoint:v;
  assert(ValidTaskPoint(r));
  return task_points[r];
}

void
Task::setTaskPoint(const unsigned index, const TASK_POINT& tp)
{
  task_points[index]= tp;
  // refresh task/set modified?
}

void
Task::setTaskIndices(const int wpindex[MAXTASKPOINTS])
{
  for (unsigned i=0; i<MAXTASKPOINTS; i++) {
    task_points[i].Index = wpindex[i];
  }
  // set modified?
}

void
Task::RotateStartPoints(const SETTINGS_COMPUTER &settings_computer,
                        const NMEA_INFO &nmea_info)
{
  if (ActiveTaskPoint>0) return;
  if (!settings.EnableMultipleStartPoints) return;

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

  RefreshTask(settings_computer, nmea_info);
}

double Task::AdjustAATTargets(double desired)
{
  int i, istart, inum;
  double av=0;
  istart = max(1,ActiveTaskPoint);
  inum=0;

  for(i=istart;i<MAXTASKPOINTS-1;i++) {
    if(ValidTaskPoint(i)&&ValidTaskPoint(i+1)
       && !task_points[i].AATTargetLocked) {
      task_points[i].AATTargetOffsetRadius =
        max(-1,min(1,
                   task_points[i].AATTargetOffsetRadius));
      av += task_points[i].AATTargetOffsetRadius;
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

  for(i=istart;i<MAXTASKPOINTS-1;i++) {
    if((task_points[i].Index >=0)
       &&(task_points[i+1].Index >=0)
       && !task_points[i].AATTargetLocked)
    {
      double d = (task_points[i].AATTargetOffsetRadius+1.0)/2.0;
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
      task_points[i].AATTargetOffsetRadius = d;
    }
  }
  // TODO RefreshTask ?

 OnExit:
  return av;
}

void
Task::CalculateAATTaskSectors(const NMEA_INFO &gps_info)
{
  int i;
  int awp = ActiveTaskPoint;

  if(!settings.AATEnabled)
    return;

  task_points[0].AATTargetOffsetRadius = 0.0;
  task_points[0].AATTargetOffsetRadial = 0.0;
  if (task_points[0].Index>=0) {
    task_points[0].AATTargetLocation = getTaskPointLocation(0);
  }

  for(i=1;i<MAXTASKPOINTS;i++) {
    if(ValidTaskPoint(i)) {
      if (!ValidTaskPoint(i+1)) {
        // This must be the final waypoint, so it's not an AAT OZ
        task_points[i].AATTargetLocation = getTaskPointLocation(i);
        continue;
      }

      if(task_points[i].AATType == AAT_SECTOR) {
        FindLatitudeLongitude (getTaskPointLocation(i),
                               task_points[i].AATStartRadial,
                               task_points[i].AATSectorRadius,
                               &task_points[i].AATStart);

        FindLatitudeLongitude (getTaskPointLocation(i),
                               task_points[i].AATFinishRadial ,
                               task_points[i].AATSectorRadius,
                               &task_points[i].AATFinish);
      }

      // JMWAAT: if locked, don't move it
      if (i<awp) {
        // only update targets for current/later waypoints
        continue;
      }

      task_points[i].AATTargetOffsetRadius =
        min(1.0, max(task_points[i].AATTargetOffsetRadius,-1.0));

      task_points[i].AATTargetOffsetRadial =
        min(90, max(-90, task_points[i].AATTargetOffsetRadial));

      double targetbearing;
      double targetrange;

      targetbearing = AngleLimit360(task_points[i].Bisector+task_points[i].AATTargetOffsetRadial);

      if(task_points[i].AATType == AAT_SECTOR) {

        //AATStartRadial
        //AATFinishRadial

        targetrange = ((task_points[i].AATTargetOffsetRadius+1.0)/2.0);

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
        targetrange = task_points[i].AATTargetOffsetRadius
          *task_points[i].AATCircleRadius;
      }

      // TODO accuracy: if i=awp and in sector, range parameter needs to
      // go from current aircraft position to projection of target
      // out to the edge of the sector

      if (InAATTurnSector(gps_info.Location, i) && (awp==i) &&
          !task_points[i].AATTargetLocked) {

        // special case, currently in AAT sector/cylinder

        double dist;
        double qdist;
        double bearing;

        // find bearing from last target through current aircraft position with offset
        DistanceBearing(task_points[i-1].AATTargetLocation,
                        gps_info.Location,
                        &qdist, &bearing);

        bearing = AngleLimit360(bearing+task_points[i].AATTargetOffsetRadial);

        dist = ((task_points[i].AATTargetOffsetRadius+1)/2.0)*
          FindInsideAATSectorDistance(gps_info.Location, i, bearing);

        // if (dist+qdist>aatdistance.LegDistanceAchieved(awp)) {
        // JMW: don't prevent target from being closer to the aircraft
        // than the best achieved, so can properly plan arrival time

        FindLatitudeLongitude (gps_info.Location,
                               bearing,
                               dist,
                               &task_points[i].AATTargetLocation);

        TargetModified = true;

        // }

      } else {

        FindLatitudeLongitude (getTaskPointLocation(i),
                               targetbearing,
                               targetrange,
                               &task_points[i].AATTargetLocation);
        TargetModified = true;

      }
    }
  }

  CalculateAATIsoLines();
  if (!targetManipEvent.test()) {
    TargetModified = false;
    // allow target dialog to detect externally changed targets
  }
}

void Task::ClearTask(void) {
  memset( &(task_points), 0, sizeof(Task_t));
  memset( &(task_start_points), 0, sizeof(Start_t));

  TaskModified = true;
  TargetModified = true;
  ClearTaskFileName();
  ActiveTaskPoint = 0;
  int i;
  for(i=0;i<MAXTASKPOINTS;i++) {
    task_points[i].Index = -1;
    task_points[i].AATSectorRadius = settings.SectorRadius; // JMW added default
    task_points[i].AATCircleRadius = settings.SectorRadius; // JMW added default
    task_points[i].AATTargetOffsetRadial = 0;
    task_points[i].AATTargetOffsetRadius = 0;
    task_points[i].AATTargetLocked = false;
    for (int j=0; j<MAXISOLINES; j++) {
      task_points[i].IsoLine_valid[j] = false;
    }
    Task_saved[i] = task_points[i].Index;
  }
  for (i=0; i<MAXSTARTPOINTS; i++) {
    task_start_points[i].Index = -1;
  }
}

/**
 * Returns whether the ActiveTaskPoint is a valid TaskPoint
 * @return True if the ActiveTaskPoint is a valid TaskPoint, False otherwise
 */
bool
Task::Valid() const
{
  return ValidTaskPoint(ActiveTaskPoint);
}

bool
Task::ValidTaskPoint(const unsigned i) const
{
  if (i>= MAXTASKPOINTS)
    return false;
  else if (task_points[i].Index<0)
    return false;
  else if (!way_points.verify_index(task_points[i].Index))
    return false;
  else
    return true;
}

double
Task::FindInsideAATSectorDistance(const GEOPOINT &location,
                                   const int taskwaypoint,
                                   const double course_bearing,
                                   const double p_found) const
{
  double max_distance;
  if(task_points[taskwaypoint].AATType == AAT_SECTOR) {
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

double
Task::FindInsideAATSectorRange(const GEOPOINT &location,
                               const int taskwaypoint,
                               const double course_bearing,
                               const double p_found) const
{

  double t_distance = FindInsideAATSectorDistance(location, taskwaypoint,
                                                  course_bearing, p_found);
  return (p_found /
          max(1,t_distance))*2-1;
}

double
Task::DoubleLegDistance(const int taskwaypoint,
                        const GEOPOINT &location) const
{
  if (taskwaypoint>0) {
    return DoubleDistance(task_points[taskwaypoint-1].AATTargetLocation,
			  location,
			  task_points[taskwaypoint+1].AATTargetLocation);
  } else {
    return Distance(location,
		    task_points[taskwaypoint+1].AATTargetLocation);
  }
}

/**
 * Returns whether a task is temporary.
 *
 * Condition 1: Task is aborted -> temporary
 * Condition 2: Task consists of just one waypoint and is not(?)
 * saved -> temporary
 * @return True if task is temporary, False otherwise
 */
bool
Task::TaskIsTemporary(void) const {
  bool retval = false;
  if (TaskAborted) {
    retval = true;
  }
  if ((task_points[0].Index>=0) && (task_points[1].Index== -1)
      && (Task_saved[0] >= 0)) {
    retval = true;
  };
  return retval;
}

void Task::BackupTask(void) {
  for (int i=0; i<=MAXTASKPOINTS; i++) {
    Task_saved[i]= task_points[i].Index;
  }
  active_waypoint_saved = ActiveTaskPoint;
  if (settings.AATEnabled) {
    aat_enabled_saved = true;
  } else {
    aat_enabled_saved = false;
  }
}


void
Task::ResumeAbortTask(const SETTINGS_COMPUTER &settings_computer,
                      const NMEA_INFO &nmea_info,
                      const int set)
{
  int i;
  unsigned active_waypoint_on_entry;
  bool task_temporary_on_entry = TaskIsTemporary();

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
      ActiveTaskPoint = 0;

      // force AAT off
      settings.AATEnabled = false;

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
      settings.AATEnabled = aat_enabled_saved;

      RefreshTask(settings_computer, nmea_info);
    }
  }

  if (active_waypoint_on_entry != ActiveTaskPoint){
    SelectedWaypoint = task_points[ActiveTaskPoint].Index;
  }

}

int
Task::getFinalWaypoint() const {
  if (TaskAborted) {
    return ActiveTaskPoint;
  }

  unsigned i= ActiveTaskPoint+1;
  while ((i<MAXTASKPOINTS) && (task_points[i].Index != -1)) {
    i++;
  }
  return i-1;
}

bool
Task::ActiveIsFinalWaypoint() const
{
  return ((int)ActiveTaskPoint == getFinalWaypoint());
}

bool
Task::InAATTurnSector(const GEOPOINT &location,
                      const int the_turnpoint) const
{
  bool retval = false;

  if (!ValidTaskPoint(the_turnpoint)) {
    return false;
  }

  double distance;
  double bearing;
  DistanceBearing(getTaskPointLocation(the_turnpoint),
                  location, &distance, &bearing);

  if(task_points[the_turnpoint].AATType ==  AAT_CIRCLE) {
    if(distance < task_points[the_turnpoint].AATCircleRadius) {
      retval = true;
    }
  } else if(distance < task_points[the_turnpoint].AATSectorRadius) {
    if (AngleInRange(task_points[the_turnpoint].AATStartRadial,
                     task_points[the_turnpoint].AATFinishRadial,
                     AngleLimit360(bearing), true)) {
      retval = true;
    }
  }

  return retval;
}


void
Task::CheckStartPointInTask(void)
{
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
      // TODO: trigger refresh, modified
    }
  }
}


void
Task::ClearStartPoints()
{
  for (int i=0; i<MAXSTARTPOINTS; i++) {
    task_start_points[i].Index = -1;
    task_start_stats[i].Active = false;
  }
  task_start_points[0].Index = task_points[0].Index;
  task_start_stats[0].Active = true;
}


void
Task::SetStartPoint(const int pointnum, const int waypointnum)
{
  if ((pointnum>=0) && (pointnum<MAXSTARTPOINTS)) {
    // TODO bug: don't add it if it's already present!
    task_start_points[pointnum].Index = waypointnum;
    task_start_stats[pointnum].Active = true;
  }
}

void
Task::advanceTaskPoint(const SETTINGS_COMPUTER &settings_computer)
{
  if(ActiveTaskPoint < MAXTASKPOINTS-1) {
    // Increment Waypoint
    if (ValidTaskPoint(ActiveTaskPoint+1)) {
      if(ActiveTaskPoint == 0)	{
        // manual start
        // TODO bug: allow restart
        // TODO bug: make this work only for manual
        /* JMW ILLEGAL
           if (Calculated().TaskStartTime==0) {
           Calculated().TaskStartTime = Basic().Time;
           }
        */
      }
      ActiveTaskPoint ++;
      AdvanceArmed = false;
      SelectedWaypoint = getWaypointIndex();
      /* JMW ILLEGAL
         Calculated().LegStartTime = Basic().Time ;
      */
    }
  }
}


void
Task::retreatTaskPoint(const SETTINGS_COMPUTER &settings_computer,
                       const NMEA_INFO &nmea_info)
{
  if(ActiveTaskPoint >0) {
    ActiveTaskPoint --;
    SelectedWaypoint = getWaypointIndex();
    /*
      XXX How do we know what the last one is?
      } else if (UpDown == -2) {
      ActiveTaskPoint = MAXTASKPOINTS;
    */
  } else {
    if (ActiveTaskPoint==0) {
      RotateStartPoints(settings_computer, nmea_info);
      // restarted task..
      //	TODO bug: not required? Calculated().TaskStartTime = 0;
      SelectedWaypoint = getWaypointIndex();
    }
  }
  //JMW illegal glide_computer.ResetEnter();
}

const SETTINGS_TASK &
Task::getSettings() const
{
  return settings;
}

void
Task::setSettings(const SETTINGS_TASK& set)
{
  settings = set;
  // user must RefreshTask after this
}

int
Task::getSelected() const
{
  return SelectedWaypoint;
}

void
Task::setSelected(const int v)
{
  if ((v==-1)&& ValidTaskPoint(ActiveTaskPoint)) {
    SelectedWaypoint = getWaypointIndex(ActiveTaskPoint);
  } else {
    SelectedWaypoint = v;
  }
}
