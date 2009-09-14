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

#include "SettingsComputer.hpp"

#define CIRCLE 0
#define SECTOR 1

typedef struct _START_POINT
{
  int Index;
  double OutBound;
  double SectorStartLat;
  double SectorStartLon;
  double SectorEndLat;
  double SectorEndLon;
} START_POINT;

typedef struct _START_POINT_STATS
{
  bool Active;
  bool InSector;
} START_POINT_STATS;

typedef struct _START_POINT_SCREEN
{
  POINT	 Start;
  POINT	 End;
} START_POINT_SCREEN;


typedef struct _TASK_POINT
{
  int Index;
  double InBound;
  double OutBound;
  double Bisector;
  double LegDistance;
  double LegBearing;
  double SectorStartLat;
  double SectorStartLon;
  double SectorEndLat;
  double SectorEndLon;
  int	 AATType;
  double AATCircleRadius;
  double AATSectorRadius;
  double AATStartRadial;
  double AATFinishRadial;
  double AATStartLat;
  double AATStartLon;
  double AATFinishLat;
  double AATFinishLon;
} TASK_POINT;

typedef struct _TASK_POINT_SCREEN
{
  POINT	 Start;
  POINT	 End;
  POINT	 Target;
  POINT	 AATStart;
  POINT	 AATFinish;
  POINT IsoLine_Screen[MAXISOLINES];
} TASK_POINT_SCREEN;


typedef struct _TASK_POINT_STATS
{
  double AATTargetOffsetRadius;
  double AATTargetOffsetRadial;
  double AATTargetLat;
  double AATTargetLon;
  bool   AATTargetLocked;
  double LengthPercent;
  double IsoLine_Latitude[MAXISOLINES];
  double IsoLine_Longitude[MAXISOLINES];
  bool IsoLine_valid[MAXISOLINES];
} TASK_POINT_STATS;


typedef TASK_POINT          Task_t        [MAXTASKPOINTS +1];
typedef TASK_POINT_SCREEN   TaskScreen_t  [MAXTASKPOINTS +1];
typedef TASK_POINT_STATS    TaskStats_t   [MAXTASKPOINTS +1];
typedef START_POINT         Start_t       [MAXSTARTPOINTS +1];
typedef START_POINT_SCREEN  StartScreen_t [MAXSTARTPOINTS +1];
typedef START_POINT_STATS   StartStats_t  [MAXSTARTPOINTS +1];


//////////////

void ReplaceWaypoint(int index, const SETTINGS_COMPUTER &settings_computer);
void InsertWaypoint(int index, const SETTINGS_COMPUTER &settings_computer,
		    bool append=false);
void SwapWaypoint(int index, const SETTINGS_COMPUTER &settings_computer);
void RemoveWaypoint(int index, const SETTINGS_COMPUTER &settings_computer);
void RemoveTaskPoint(int index, const SETTINGS_COMPUTER &settings_computer);
void FlyDirectTo(int index, const SETTINGS_COMPUTER &settings_computer);
double AdjustAATTargets(double desired);
void RefreshTaskWaypoint(int i);
void RefreshTask(const SETTINGS_COMPUTER &settings_computer);
void CalculateTaskSectors(void);
bool WaypointInTask(const int ind);

void ClearTask(void);
void RotateStartPoints(const SETTINGS_COMPUTER &settings_computer);

bool ValidTaskPoint(const int i);
bool ValidTask();
bool ValidWayPoint(const int i);

double FindInsideAATSectorRange(double latitude,
                                double longitude,
                                int taskwaypoint,
                                double course_bearing,
                                double p_found);
double FindInsideAATSectorDistance(double latitude,
                                double longitude,
                                int taskwaypoint,
                                double course_bearing,
                                double p_found=0.0);

double DoubleLegDistance(int taskwaypoint,
                         double longitude,
                         double latitude);

void CalculateAATIsoLines(void);

void DefaultTask(const SETTINGS_COMPUTER &settings);

void ResumeAbortTask(const SETTINGS_COMPUTER &settings_computer,
		     int set = 0);

bool TaskIsTemporary(void);
int  getFinalWaypoint(void);
bool ActiveIsFinalWaypoint(void);
bool IsFinalWaypoint(void);

bool InAATTurnSector(const double longitude, const double latitude,
		     const int the_turnpoint);

bool isTaskModified();
void SetTaskModified(const bool set=true);
bool isTargetModified();
void SetTargetModified(const bool set=true);
bool isTaskAborted();

void CheckStartPointInTask(void);
void ClearStartPoints(void);
void SetStartPoint(const int pointnum, const int waypointnum);

#endif
