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

#if !defined(XCSOAR_TASK_H)
#define XCSOAR_TASK_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Sizes.h"

#include "GeoPoint.hpp"
#include "SettingsComputer.hpp"
#include "WayPoint.hpp"

#define CIRCLE 0
#define SECTOR 1

typedef struct _START_POINT
{
  int Index;
  double OutBound;
  GEOPOINT SectorStart;
  GEOPOINT SectorEnd;
} START_POINT;

typedef struct _START_POINT_STATS
{
  bool Active;
  bool InSector;
} START_POINT_STATS;

typedef struct _START_POINT_SCREEN
{
  POINT	 SectorStart;
  POINT	 SectorEnd;
} START_POINT_SCREEN;


typedef struct _TASK_POINT
{
  int Index;
  double InBound;
  double OutBound;
  double Bisector;
  double LegDistance;
  double LegBearing;
  GEOPOINT SectorStart;
  GEOPOINT SectorEnd;
  int	 AATType;
  double AATCircleRadius;
  double AATSectorRadius;
  double AATStartRadial;
  double AATFinishRadial;
  GEOPOINT AATStart;
  GEOPOINT AATFinish;

  // from stats
  double AATTargetOffsetRadius;
  double AATTargetOffsetRadial;
  GEOPOINT AATTargetLocation;
  bool   AATTargetLocked;
  double LengthPercent;
  GEOPOINT IsoLine_Location[MAXISOLINES];
  bool IsoLine_valid[MAXISOLINES];

} TASK_POINT;

typedef struct _TASK_POINT_SCREEN
{
  POINT	 SectorStart;
  POINT	 SectorEnd;
  POINT	 Target;
  POINT	 AATStart;
  POINT	 AATFinish;
  POINT IsoLine_Screen[MAXISOLINES];
} TASK_POINT_SCREEN;


typedef TASK_POINT Task_t[MAXTASKPOINTS +1];
typedef TASK_POINT_SCREEN TaskScreen_t[MAXTASKPOINTS +1];
typedef START_POINT Start_t[MAXSTARTPOINTS +1];
typedef START_POINT_SCREEN StartScreen_t[MAXSTARTPOINTS +1];
typedef START_POINT_STATS StartStats_t[MAXSTARTPOINTS +1];
struct NMEA_INFO;

class RelativeTaskPointVisitor;
class AbsoluteTaskPointVisitor;
class RelativeTaskLegVisitor;
class AbsoluteTaskLegVisitor;



//////////////

class Task {
public:
  Task();
public:
  void RefreshTask(const SETTINGS_COMPUTER &settings_computer);

  void ReplaceWaypoint(const int index, 
                               const SETTINGS_COMPUTER &settings_computer);
  void InsertWaypoint(const int index, 
                              const SETTINGS_COMPUTER &settings_computer,
                              bool append=false);
  void SwapWaypoint(const int index, 
                            const SETTINGS_COMPUTER &settings_computer);
  void RemoveWaypoint(const int index, 
                              const SETTINGS_COMPUTER &settings_computer);
  void RemoveTaskPoint(const int index, 
                               const SETTINGS_COMPUTER &settings_computer);
  void FlyDirectTo(const int index, 
                           const SETTINGS_COMPUTER &settings_computer);

  void advanceTaskPoint(const SETTINGS_COMPUTER &settings_computer);
  void retreatTaskPoint(const SETTINGS_COMPUTER &settings_computer);

  void ClearTask(void);
  void RotateStartPoints(const SETTINGS_COMPUTER &settings_computer);
  void DefaultTask(const SETTINGS_COMPUTER &settings);
  void ResumeAbortTask(const SETTINGS_COMPUTER &settings_computer,
                               const int set = 0);
  void CheckStartPointInTask(void);
  void ClearStartPoints(void);
  void SetStartPoint(const int pointnum, const int waypointnum);

  // AAT functions
  double AdjustAATTargets(double desired);
  const double FindInsideAATSectorRange(const GEOPOINT &location,
                                        const int taskwaypoint,
                                        const double course_bearing,
                                        const double p_found) const;
  const double FindInsideAATSectorDistance(const GEOPOINT &location,
                                           const int taskwaypoint,
                                           const double course_bearing,
                                           const double p_found=0.0) const;
  const bool isTaskModified() const;
  void SetTaskModified(const bool set=true);
  const bool isTargetModified() const;
  void SetTargetModified(const bool set=true);
  const bool InAATTurnSector(const GEOPOINT &location, 
                             const int the_turnpoint) const;

  // queries
  const bool ValidTaskPoint(const unsigned i) const;
  const bool Valid() const;
  const double DoubleLegDistance(const int taskwaypoint, 
                                 const GEOPOINT &location) const;
  const bool TaskIsTemporary(void) const;
  const int  getFinalWaypoint(void) const;
  const bool ActiveIsFinalWaypoint(void) const;
  const bool isTaskAborted() const;
  const GEOPOINT& getTaskPointLocation(const unsigned i) const;
  const GEOPOINT& getActiveLocation() const;
  const GEOPOINT& getTargetLocation(const int v=-1) const;

  // file load/save
  void LoadNewTask(const TCHAR *FileName,
                   const SETTINGS_COMPUTER &settings_computer);
  void SaveTask(const TCHAR *FileName);
  void SaveDefaultTask(void);
  const TCHAR* getTaskFilename() const;
  void ClearTaskFileName();
  bool LoadTaskWaypoints(FILE *file);

  //
  unsigned  ActiveTaskPoint;

  const unsigned getActiveIndex() const 
  { return ActiveTaskPoint; }

  void setActiveIndex(unsigned i) {
    if (ValidTaskPoint(i)) {
      ActiveTaskPoint = i;
    }
  }

  const TASK_POINT& getTaskPoint(const int v=-1) const;
  void setTaskPoint(const unsigned index, const TASK_POINT& tp);

  const WAYPOINT& getWaypoint(const int v=-1) const;
  const int getWaypointIndex(const int v=-1) const;

  void setTaskIndices(const int wpindex[MAXTASKPOINTS]);

  // scan routines
  void scan_point_forward(RelativeTaskPointVisitor &visitor);
  void scan_point_forward(AbsoluteTaskPointVisitor &visitor);
  void scan_leg_forward(RelativeTaskLegVisitor &visitor);
  void scan_leg_forward(AbsoluteTaskLegVisitor &visitor);
  void scan_leg_reverse(RelativeTaskLegVisitor &visitor);
  void scan_leg_reverse(AbsoluteTaskLegVisitor &visitor);

protected:
 Task_t         task_points;

private:
  void ResetTaskWaypoint(int j);
  void CalculateAATTaskSectors(const NMEA_INFO &gps_info);
  void RefreshTask_Visitor(const SETTINGS_COMPUTER &settings_computer);
  void BackupTask(void);
  void CalculateAATIsoLines(void);
};

//////

#include "Protection.hpp"

class TaskSafe {
private:
  Task _task;
public:
  void RefreshTask(const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.RefreshTask(settings_computer);
  };
  void ReplaceWaypoint(const int index, 
                       const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.ReplaceWaypoint(index, settings_computer);
  }
  void InsertWaypoint(const int index, 
                      const SETTINGS_COMPUTER &settings_computer,
                      bool append=false)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.InsertWaypoint(index, settings_computer, append);
  }
  void SwapWaypoint(const int index, 
                            const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.SwapWaypoint(index, settings_computer);
  }
  void RemoveWaypoint(const int index, 
                      const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.RemoveWaypoint(index, settings_computer);
  }
  void RemoveTaskPoint(const int index, 
                       const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.RemoveTaskPoint(index, settings_computer);
  }
  void FlyDirectTo(const int index, 
                           const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.FlyDirectTo(index, settings_computer);
  }

  void advanceTaskPoint(const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.advanceTaskPoint(settings_computer);
  }
  void retreatTaskPoint(const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.retreatTaskPoint(settings_computer);
  }


  void ClearTask(void)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.ClearTask();
  }
  void RotateStartPoints(const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.RotateStartPoints(settings_computer);
  }
  void DefaultTask(const SETTINGS_COMPUTER &settings)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.DefaultTask(settings);
  }
  void ResumeAbortTask(const SETTINGS_COMPUTER &settings_computer,
                       const int set = 0)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.ResumeAbortTask(settings_computer, set);
  }

  void CheckStartPointInTask(void)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.CheckStartPointInTask();
  }

  void ClearStartPoints(void)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.ClearStartPoints();
  }

  void SetStartPoint(const int pointnum, const int waypointnum)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.SetStartPoint(pointnum, waypointnum);
  }


  // AAT functions
  double AdjustAATTargets(double desired) {
    // write
    ScopeLock protect(mutexTaskData);
    return _task.AdjustAATTargets(desired);
  }

//////
  const double FindInsideAATSectorRange(const GEOPOINT &location,
                                          const int taskwaypoint,
                                          const double course_bearing,
                                          const double p_found) const
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.FindInsideAATSectorRange(location, taskwaypoint,
                                          course_bearing, p_found);
  }
  const double FindInsideAATSectorDistance(const GEOPOINT &location,
                                           const int taskwaypoint,
                                           const double course_bearing,
                                           const double p_found=0.0) const
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.FindInsideAATSectorDistance(location, taskwaypoint,
                                             course_bearing, p_found);
  }
  const bool isTaskModified() const 
  {
    // read
    ScopeLock protect(mutexTaskData);
    return _task.isTaskModified();
  }

  void SetTaskModified(const bool set=true)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.SetTaskModified(set);
  }

  const bool isTargetModified() const 
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.isTargetModified();
  }

  void SetTargetModified(const bool set=true)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.SetTargetModified(set);
  }

  const bool InAATTurnSector(const GEOPOINT &location, 
                             const int the_turnpoint) const
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.InAATTurnSector(location, the_turnpoint);
  }

  // queries
  const bool verify_index(const unsigned i) const
  { // read
    // alias
    return ValidTaskPoint(i);
  }

  const unsigned getActiveIndex() const 
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.getActiveIndex();
  }
  void setActiveIndex(unsigned i) 
  { // write
    ScopeLock protect(mutexTaskData);
    return _task.setActiveIndex(i);
  }

  const TASK_POINT& getTaskPoint(const int v=-1) const
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.getTaskPoint(v);
  }

  void setTaskPoint(const unsigned index, const TASK_POINT& tp)
  { // write
    ScopeLock protect(mutexTaskData);
    _task.setTaskPoint(index, tp);
  }

  void setTaskIndices(const int wpindex[MAXTASKPOINTS]) 
  { // write
    ScopeLock protect(mutexTaskData);
    _task.setTaskIndices(wpindex);
  }

  const int getWaypointIndex(const int v=-1) const
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.getWaypointIndex(v);
  }
  const WAYPOINT& getWaypoint(const int v=-1) const
  {
    ScopeLock protect(mutexTaskData);
    return _task.getWaypoint(v);
  }

  const bool ValidTaskPoint(const unsigned i) const
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.ValidTaskPoint(i);
  }
  const bool Valid() const 
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.Valid();
  }

  const double DoubleLegDistance(const int taskwaypoint, // read
                                 const GEOPOINT &location) const 
  {
    ScopeLock protect(mutexTaskData);
    return _task.DoubleLegDistance(taskwaypoint, location);
  }
  const bool TaskIsTemporary(void) const 
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.TaskIsTemporary();
  }
  const int  getFinalWaypoint(void) const 
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.getFinalWaypoint();
  }
  const bool ActiveIsFinalWaypoint(void) const 
  { //read
    ScopeLock protect(mutexTaskData);
    return _task.ActiveIsFinalWaypoint();
  }
  const bool isTaskAborted() const 
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.isTaskAborted();
  }

  const GEOPOINT &getTaskPointLocation(const unsigned i) const 
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.getTaskPointLocation(i);
  }

  const GEOPOINT &getActiveLocation() const 
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.getActiveLocation();
  }
  const GEOPOINT &getTargetLocation(const int v=-1) const 
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.getTargetLocation(v);
  }

  // file load/save
  void LoadNewTask(const TCHAR *FileName,
                   const SETTINGS_COMPUTER &settings_computer) // write
  {
    ScopeLock protect(mutexTaskData);
    _task.LoadNewTask(FileName, settings_computer);
  }

  void SaveTask(const TCHAR *FileName) // write
  {
    ScopeLock protect(mutexTaskData);
    _task.SaveTask(FileName);
  }

  void SaveDefaultTask(void) 
  { // write
    ScopeLock protect(mutexTaskData);
    _task.SaveDefaultTask();
  }

  const TCHAR* getTaskFilename() const 
  { // read
    ScopeLock protect(mutexTaskData);
    return _task.getTaskFilename();
  }
  void ClearTaskFileName() // write
  {
    ScopeLock protect(mutexTaskData);
    _task.ClearTaskFileName();
  }
 
  // potentially write

  void scan_point_forward(RelativeTaskPointVisitor &visitor) {
    ScopeLock protect(mutexTaskData);
    _task.scan_point_forward(visitor);
  };

  void scan_point_forward(AbsoluteTaskPointVisitor &visitor) {
    ScopeLock protect(mutexTaskData);
    _task.scan_point_forward(visitor);
  };

  void scan_leg_forward(RelativeTaskLegVisitor &visitor) {
    ScopeLock protect(mutexTaskData);
    _task.scan_leg_forward(visitor);
  };

  void scan_leg_forward(AbsoluteTaskLegVisitor &visitor) {
    ScopeLock protect(mutexTaskData);
    _task.scan_leg_forward(visitor);
  };

  void scan_leg_reverse(RelativeTaskLegVisitor &visitor) {
    ScopeLock protect(mutexTaskData);
    _task.scan_leg_reverse(visitor);
  };

  void scan_leg_reverse(AbsoluteTaskLegVisitor &visitor) {
    ScopeLock protect(mutexTaskData);
    _task.scan_leg_reverse(visitor);
  };
};


extern TaskSafe task;

#endif
