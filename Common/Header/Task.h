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

#if !defined(AFX_TASK_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_TASK_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include <windows.h>
#include "Sizes.h"

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
  POINT	 Start;
  POINT	 End;
  bool Active;
  bool InSector;
} START_POINT;


typedef struct _TASK_POINT
{
  int Index;
  double InBound;
  double OutBound;
  double Bisector;
  double Leg;
  double SectorStartLat;
  double SectorStartLon;
  double SectorEndLat;
  double SectorEndLon;
  POINT	 Start;
  POINT	 End;
  int	 AATType;
  double AATCircleRadius;
  double AATSectorRadius;
  double AATStartRadial;
  double AATFinishRadial;
  double AATStartLat;
  double AATStartLon;
  double AATFinishLat;
  double AATFinishLon;
  POINT	 AATStart;
  POINT	 AATFinish;
  double AATTargetOffsetRadius;
  double AATTargetOffsetRadial;
  double AATTargetLat;
  double AATTargetLon;
  POINT	 Target;
  bool   AATTargetLocked;
}TASK_POINT;

typedef TASK_POINT Task_t[MAXTASKPOINTS +1];
typedef START_POINT Start_t[MAXSTARTPOINTS +1];

typedef struct _TASKSTATS_POINT
{
  double LengthPercent;
  double IsoLine_Latitude[MAXISOLINES];
  double IsoLine_Longitude[MAXISOLINES];
  bool IsoLine_valid[MAXISOLINES];
  POINT IsoLine_Screen[MAXISOLINES];
}TASKSTATS_POINT;

typedef TASKSTATS_POINT TaskStats_t[MAXTASKPOINTS +1];

extern bool EnableMultipleStartPoints;
extern bool TaskModified;
extern bool TargetModified;
extern int StartHeightRef;
extern TCHAR LastTaskFileName[MAX_PATH];

void ReplaceWaypoint(int index);
void InsertWaypoint(int index, bool append=false);
void SwapWaypoint(int index);
void RemoveWaypoint(int index);
void RemoveTaskPoint(int index);
void FlyDirectTo(int index);
double AdjustAATTargets(double desired);
void RefreshTaskWaypoint(int i);
void RefreshTask(void);
void CalculateTaskSectors(void);
void CalculateAATTaskSectors(void);

void guiStartLogger(bool noAsk = false);
void guiStopLogger(bool noAsk = false);
void guiToggleLogger(bool noAsk = false);

void LoadNewTask(TCHAR *FileName);
void LoadTask(TCHAR *FileName,HWND hDlg);
void SaveTask(TCHAR *FileName);
void DefaultTask(void);
void ClearTask(void);
void RotateStartPoints(void);
bool ValidTaskPoint(int i);
bool ValidWayPoint(int i);

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

void SaveDefaultTask(void);

void ResumeAbortTask(int set = 0);

bool TaskIsTemporary(void);

#endif
