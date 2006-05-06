#if !defined(AFX_TASK_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_TASK_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "sizes.h"

#define CIRCLE 0
#define SECTOR 1

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
}TASK_POINT;

typedef TASK_POINT Task_t[MAXTASKPOINTS +1];

typedef struct _TASKSTATS_POINT
{
  double LengthPercent;
}TASKSTATS_POINT;

typedef TASKSTATS_POINT TaskStats_t[MAXTASKPOINTS +1];


void ReplaceWaypoint(int index);
void InsertWaypoint(int index);
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



#endif
