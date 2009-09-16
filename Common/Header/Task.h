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
  void RefreshTask(const SETTINGS_COMPUTER &settings_computer);

  void ReplaceWaypoint(const int index, 
                       const SETTINGS_COMPUTER &settings_computer);
  void InsertWaypoint(const int index, 
                      const SETTINGS_COMPUTER &settings_computer,
                      bool append=false);
  void SwapWaypoint(int index, const SETTINGS_COMPUTER &settings_computer);
  void RemoveWaypoint(const int index, 
                      const SETTINGS_COMPUTER &settings_computer);
  void RemoveTaskPoint(const int index, 
                       const SETTINGS_COMPUTER &settings_computer);
  void FlyDirectTo(int index, const SETTINGS_COMPUTER &settings_computer);

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
  double FindInsideAATSectorRange(const GEOPOINT &location,
                                  const int taskwaypoint,
                                  const double course_bearing,
                                  const double p_found);
  double FindInsideAATSectorDistance(const GEOPOINT &location,
                                     const int taskwaypoint,
                                     const double course_bearing,
                                     const double p_found=0.0);
  bool isTaskModified();
  void SetTaskModified(const bool set=true);
  bool isTargetModified();
  void SetTargetModified(const bool set=true);
  bool InAATTurnSector(const GEOPOINT &location, const int the_turnpoint);

  // queries
  bool ValidTaskPoint(const int i);
  bool Valid();
  double DoubleLegDistance(const int taskwaypoint, 
                           const GEOPOINT &location);
  bool TaskIsTemporary(void);
  int  getFinalWaypoint(void);
  bool ActiveIsFinalWaypoint(void);
  bool IsFinalWaypoint(void);
  bool isTaskAborted();

  // file load/save
  void LoadNewTask(const TCHAR *FileName,
                   const SETTINGS_COMPUTER &settings_computer);
  void SaveTask(const TCHAR *FileName);
  void SaveDefaultTask(void);
  const TCHAR* getTaskFilename();
  void ClearTaskFileName();

private:
  void ResetTaskWaypoint(int j);
  void CalculateAATTaskSectors(const NMEA_INFO &gps_info);
  void RefreshTask_Visitor(const SETTINGS_COMPUTER &settings_computer);
  void BackupTask(void);
  void CalculateAATIsoLines(void);
};

extern Task task;

#endif
