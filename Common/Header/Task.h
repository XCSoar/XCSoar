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


typedef struct _TASK_POINT_STATS
{
  double AATTargetOffsetRadius;
  double AATTargetOffsetRadial;
  GEOPOINT AATTargetLocation;
  bool   AATTargetLocked;
  double LengthPercent;
  GEOPOINT IsoLine_Location[MAXISOLINES];
  bool IsoLine_valid[MAXISOLINES];
} TASK_POINT_STATS;


typedef TASK_POINT Task_t[MAXTASKPOINTS +1];
typedef TASK_POINT_SCREEN TaskScreen_t[MAXTASKPOINTS +1];
typedef TASK_POINT_STATS TaskStats_t[MAXTASKPOINTS +1];
typedef START_POINT Start_t[MAXSTARTPOINTS +1];
typedef START_POINT_SCREEN StartScreen_t[MAXSTARTPOINTS +1];
typedef START_POINT_STATS StartStats_t[MAXSTARTPOINTS +1];
struct NMEA_INFO;

//////////////

class Task {
public:
  virtual void RefreshTask(const SETTINGS_COMPUTER &settings_computer);

  virtual void ReplaceWaypoint(const int index, 
                               const SETTINGS_COMPUTER &settings_computer);
  virtual void InsertWaypoint(const int index, 
                              const SETTINGS_COMPUTER &settings_computer,
                              bool append=false);
  virtual void SwapWaypoint(const int index, 
                            const SETTINGS_COMPUTER &settings_computer);
  virtual void RemoveWaypoint(const int index, 
                              const SETTINGS_COMPUTER &settings_computer);
  virtual void RemoveTaskPoint(const int index, 
                               const SETTINGS_COMPUTER &settings_computer);
  virtual void FlyDirectTo(const int index, 
                           const SETTINGS_COMPUTER &settings_computer);

  virtual void advanceTaskPoint(const SETTINGS_COMPUTER &settings_computer);
  virtual void retreatTaskPoint(const SETTINGS_COMPUTER &settings_computer);

  virtual void ClearTask(void);
  virtual void RotateStartPoints(const SETTINGS_COMPUTER &settings_computer);
  virtual void DefaultTask(const SETTINGS_COMPUTER &settings);
  virtual void ResumeAbortTask(const SETTINGS_COMPUTER &settings_computer,
                               const int set = 0);
  virtual void CheckStartPointInTask(void);
  virtual void ClearStartPoints(void);
  virtual void SetStartPoint(const int pointnum, const int waypointnum);

  // AAT functions
  virtual double AdjustAATTargets(double desired);
  virtual double FindInsideAATSectorRange(const GEOPOINT &location,
                                  const int taskwaypoint,
                                  const double course_bearing,
                                  const double p_found);
  virtual double FindInsideAATSectorDistance(const GEOPOINT &location,
                                     const int taskwaypoint,
                                     const double course_bearing,
                                     const double p_found=0.0);
  virtual bool isTaskModified();
  virtual void SetTaskModified(const bool set=true);
  virtual bool isTargetModified();
  virtual void SetTargetModified(const bool set=true);
  virtual bool InAATTurnSector(const GEOPOINT &location, const int the_turnpoint);

  // queries
  virtual bool ValidTaskPoint(const int i);
  virtual bool Valid();
  virtual double DoubleLegDistance(const int taskwaypoint, 
                           const GEOPOINT &location);
  virtual bool TaskIsTemporary(void);
  virtual int  getFinalWaypoint(void);
  virtual bool ActiveIsFinalWaypoint(void);
  virtual bool isTaskAborted();
  virtual const GEOPOINT &getTaskPointLocation(const unsigned i);
  virtual const GEOPOINT &getActiveLocation();

  // file load/save
  virtual void LoadNewTask(const TCHAR *FileName,
                   const SETTINGS_COMPUTER &settings_computer);
  virtual void SaveTask(const TCHAR *FileName);
  virtual void SaveDefaultTask(void);
  virtual const TCHAR* getTaskFilename();
  virtual void ClearTaskFileName();

private:
  void ResetTaskWaypoint(int j);
  void CalculateAATTaskSectors(const NMEA_INFO &gps_info);
  void RefreshTask_Visitor(const SETTINGS_COMPUTER &settings_computer);
  void BackupTask(void);
  void CalculateAATIsoLines(void);
};


//////

#include "Protection.hpp"

class TaskSafe: private Task {
public:
  virtual void RefreshTask(const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::RefreshTask(settings_computer);
  };
  virtual void ReplaceWaypoint(const int index, 
                       const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::ReplaceWaypoint(index, settings_computer);
  }
  virtual void InsertWaypoint(const int index, 
                      const SETTINGS_COMPUTER &settings_computer,
                      bool append=false)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::InsertWaypoint(index, settings_computer, append);
  }
  virtual void SwapWaypoint(const int index, 
                            const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::SwapWaypoint(index, settings_computer);
  }
  virtual void RemoveWaypoint(const int index, 
                      const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::RemoveWaypoint(index, settings_computer);
  }
  virtual void RemoveTaskPoint(const int index, 
                       const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::RemoveTaskPoint(index, settings_computer);
  }
  virtual void FlyDirectTo(const int index, 
                           const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::FlyDirectTo(index, settings_computer);
  }

  virtual void advanceTaskPoint(const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::advanceTaskPoint(settings_computer);
  }
  virtual void retreatTaskPoint(const SETTINGS_COMPUTER &settings_computer)
  {
    ScopeLock protect(mutexTaskData);
    Task::retreatTaskPoint(settings_computer);
  }


  virtual void ClearTask(void)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::ClearTask();
  }
  virtual void RotateStartPoints(const SETTINGS_COMPUTER &settings_computer)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::RotateStartPoints(settings_computer);
  }
  virtual void DefaultTask(const SETTINGS_COMPUTER &settings)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::DefaultTask(settings);
  }
  virtual void ResumeAbortTask(const SETTINGS_COMPUTER &settings_computer,
                       const int set = 0)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::ResumeAbortTask(settings_computer, set);
  }

  virtual void CheckStartPointInTask(void)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::CheckStartPointInTask();
  }

  virtual void ClearStartPoints(void)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::ClearStartPoints();
  }

  virtual void SetStartPoint(const int pointnum, const int waypointnum)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::SetStartPoint(pointnum, waypointnum);
  }


  // AAT functions
  virtual double AdjustAATTargets(double desired) {
    // write
    ScopeLock protect(mutexTaskData);
    return Task::AdjustAATTargets(desired);
  }

//////
  virtual double FindInsideAATSectorRange(const GEOPOINT &location,
                                          const int taskwaypoint,
                                          const double course_bearing,
                                          const double p_found) 
  { // read
    ScopeLock protect(mutexTaskData);
    return Task::FindInsideAATSectorRange(location, taskwaypoint,
                                          course_bearing, p_found);
  }
  virtual double FindInsideAATSectorDistance(const GEOPOINT &location,
                                             const int taskwaypoint,
                                             const double course_bearing,
                                             const double p_found=0.0) 
  { // read
    ScopeLock protect(mutexTaskData);
    return Task::FindInsideAATSectorDistance(location, taskwaypoint,
                                             course_bearing, p_found);
  }
  virtual bool isTaskModified() {
    // read
    ScopeLock protect(mutexTaskData);
    return Task::isTaskModified();
  }

  virtual void SetTaskModified(const bool set=true)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::SetTaskModified(set);
  }

  virtual bool isTargetModified() { // read
    ScopeLock protect(mutexTaskData);
    return Task::isTargetModified();
  }
  virtual void SetTargetModified(const bool set=true)
  { // write
    ScopeLock protect(mutexTaskData);
    Task::SetTargetModified(set);
  }

  virtual bool InAATTurnSector(const GEOPOINT &location, 
                               const int the_turnpoint) 
  { // read
    ScopeLock protect(mutexTaskData);
    return Task::InAATTurnSector(location, the_turnpoint);
  }

  // queries
  virtual bool verify_index(const int i) { // read
    // alias
    return ValidTaskPoint(i);
  }
  virtual bool ValidTaskPoint(const int i) { // read
    ScopeLock protect(mutexTaskData);
    return Task::ValidTaskPoint(i);
  }
  virtual bool Valid() { // read
    ScopeLock protect(mutexTaskData);
    return Task::Valid();
  }
  virtual double DoubleLegDistance(const int taskwaypoint, // read
                                   const GEOPOINT &location) {
    ScopeLock protect(mutexTaskData);
    return Task::DoubleLegDistance(taskwaypoint, location);
  }
  virtual bool TaskIsTemporary(void) { // read
    ScopeLock protect(mutexTaskData);
    return Task::TaskIsTemporary();
  }
  virtual int  getFinalWaypoint(void) { // read
    ScopeLock protect(mutexTaskData);
    return Task::getFinalWaypoint();
  }
  virtual bool ActiveIsFinalWaypoint(void) { //read
    ScopeLock protect(mutexTaskData);
    return Task::ActiveIsFinalWaypoint();
  }
  virtual bool isTaskAborted() { // read
    ScopeLock protect(mutexTaskData);
    return Task::isTaskAborted();
  }

  virtual const GEOPOINT &getTaskPointLocation(const unsigned i) {
    ScopeLock protect(mutexTaskData);
    return Task::getTaskPointLocation(i);
  }

  virtual const GEOPOINT &getActiveLocation() {
    ScopeLock protect(mutexTaskData);
    return Task::getActiveLocation();
  }

  // file load/save
  virtual void LoadNewTask(const TCHAR *FileName,
                           const SETTINGS_COMPUTER &settings_computer) // write
  {
    ScopeLock protect(mutexTaskData);
    Task::LoadNewTask(FileName, settings_computer);
  }

  virtual void SaveTask(const TCHAR *FileName) // write
  {
    ScopeLock protect(mutexTaskData);
    Task::SaveTask(FileName);
  }

  virtual void SaveDefaultTask(void) // write
  {
    ScopeLock protect(mutexTaskData);
    Task::SaveDefaultTask();
  }

  virtual const TCHAR* getTaskFilename() // read
  {
    ScopeLock protect(mutexTaskData);
    return Task::getTaskFilename();
  }
  virtual void ClearTaskFileName() // write
  {
    ScopeLock protect(mutexTaskData);
    Task::ClearTaskFileName();
  }
};


extern TaskSafe task;

#endif
