/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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
#include "stdafx.h"
#include "Task.h"
#include "Logger.h"
#include "XCSoar.h"
#include "Utils.h"
#include "externs.h"

bool EnableMultipleStartPoints = false;

void FlyDirectTo(int index) {
  if (!CheckDeclaration())
    return;

  ActiveWayPoint = -1; AATEnabled = FALSE;
  for(int j=0;j<MAXTASKPOINTS;j++)
  {
    Task[j].Index = -1;
    Task[j].AATTargetOffsetRadius = 0.0;
  }
  Task[0].Index = index;
  Task[0].AATTargetOffsetRadius = 0.0;
  ActiveWayPoint = 0;
  RefreshTask();
}


// Swaps waypoint at current index with next one.
void SwapWaypoint(int index) {
  if (!CheckDeclaration())
    return;

  if (index<0) {
    return;
  }
  if (index+1>= MAXTASKPOINTS-1) {
    return;
  }
  if ((Task[index].Index != -1)&&(Task[index+1].Index != -1)) {
    TASK_POINT tmpPoint;
    tmpPoint = Task[index];
    Task[index] = Task[index+1];
    Task[index+1] = tmpPoint;
  }
  RefreshTask();
}


// Inserts a waypoint into the task, in the
// position of the ActiveWaypoint.  If append=true, insert at end of the
// task.
void InsertWaypoint(int index, bool append) {
  if (!CheckDeclaration())
    return;

  int i;

  if (ActiveWayPoint<0) {
    ActiveWayPoint = 0;
    Task[ActiveWayPoint].Index = index;
    Task[ActiveWayPoint].AATTargetOffsetRadius= 0.0;
    return;
  }

  if (Task[MAXTASKPOINTS-1].Index != -1) {
    // No room for any more task points!
    MessageBoxX(hWndMapWindow,
      gettext(TEXT("Too many waypoints in task!")),
      gettext(TEXT("Insert Waypoint")),
      MB_OK|MB_ICONEXCLAMATION);

    return;
  }

  int indexInsert;
  if (append) {
    for (i=ActiveWayPoint; i<MAXTASKPOINTS-2; i++) {
      if (Task[i+1].Index<0) {
	Task[i+1].Index = index;
	Task[i+1].AATTargetOffsetRadius= 0.0;
	break;
      }
    }
  } else {
    indexInsert = ActiveWayPoint;

    // Shuffle ActiveWaypoint and all later task points
    // to the right by one position
    for (i=MAXTASKPOINTS-1; i>indexInsert; i--) {
      Task[i].Index = Task[i-1].Index;
      Task[i].AATTargetOffsetRadius= Task[i-1].AATTargetOffsetRadius;
    }
    // Insert new point and update task details
    Task[indexInsert].Index = index;
    Task[indexInsert].AATTargetOffsetRadius= 0.0;
  }

  RefreshTask();

}

// Create a default task to home at startup if no task is present
void DefaultTask(void) {
  if ((Task[0].Index == -1)||(ActiveWayPoint==-1)) {
    if (HomeWaypoint != -1) {
      Task[0].Index = HomeWaypoint;
      ActiveWayPoint = 0;
    }
  }
  RefreshTask();
}


// RemoveTaskpoint removes a single waypoint
// from the current task.  index specifies an entry
// in the Task[] array - NOT a waypoint index.
//
// If you call this function, you MUST deal with
// correctly setting ActiveWayPoint yourself!
void RemoveTaskPoint(int index) {
  if (!CheckDeclaration())
    return;

  int i;

  if (index < 0 || index >= MAXTASKPOINTS) {
    return; // index out of bounds
  }

  if (Task[index].Index == -1) {
    return; // There's no WP at this location
  }

  // Shuffle all later taskpoints to the left to
  // fill the gap
  for (i=index; i<MAXTASKPOINTS-1; ++i) {
    Task[i].Index = Task[i+1].Index;
    Task[i].AATTargetOffsetRadius= Task[i+1].AATTargetOffsetRadius;
  }
  Task[MAXTASKPOINTS-1].Index = -1;
  Task[MAXTASKPOINTS-1].AATTargetOffsetRadius= 0.0;

  RefreshTask();

}


// Index specifies a waypoint in the WP list
// It won't necessarily be a waypoint that's
// in the task
void RemoveWaypoint(int index) {
  int i;

  if (!CheckDeclaration())
    return;

  if (ActiveWayPoint<0) {
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

  // Search forward first
  i = ActiveWayPoint;
  while ((i < MAXTASKPOINTS) && (Task[i].Index != index)) {
    ++i;
  }

  if (i < MAXTASKPOINTS) {
    // Found WP, so remove it
    RemoveTaskPoint(i);

    if (Task[ActiveWayPoint].Index == -1) {
      // We've just removed the last task point and it was
      // active at the time
      ActiveWayPoint--;
    }

  } else {
    // Didn't find WP, so search backwards

    i = ActiveWayPoint;
    do {
      --i;
    } while (i >= 0 && Task[i].Index != index);

    if (i >= 0) {
      // Found WP, so remove it
      RemoveTaskPoint(i);
      ActiveWayPoint--;

    } else {
      // WP not found, so ask user if they want to
      // remove the active WP
      int ret = MessageBoxX(hWndMapWindow,
        gettext(TEXT("Chosen Waypoint not in current task.\nRemove active WayPoint?")),
        gettext(TEXT("Remove Waypoint")),
        MB_YESNO|MB_ICONQUESTION);

      if (ret == IDYES) {
        RemoveTaskPoint(ActiveWayPoint);
        if (Task[ActiveWayPoint].Index == -1) {
          // Active WayPoint was last in the list so is currently
          // invalid.
          ActiveWayPoint--;
        }
      }
    }
  }
  RefreshTask();

}


void ReplaceWaypoint(int index) {
  if (!CheckDeclaration())
    return;

  // ARH 26/06/05 Fixed array out-of-bounds bug
  if (ActiveWayPoint>=0) {

    Task[ActiveWayPoint].Index = index;
    Task[ActiveWayPoint].AATTargetOffsetRadius= 0.0;
  } else {

    // Insert a new waypoint since there's
    // nothing to replace
    ActiveWayPoint=0;
    Task[ActiveWayPoint].Index = index;
    Task[ActiveWayPoint].AATTargetOffsetRadius= 0.0;
  }
  RefreshTask();
}


void RefreshTask() {
  double lengthtotal = 0.0;
  int i;

  if ((ActiveWayPoint<0)&&(Task[0].Index>=0)) {
    ActiveWayPoint=0;
  }

  // Only need to refresh info where the removal happened
  // as the order of other taskpoints hasn't changed
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (Task[i].Index != -1) {
      RefreshTaskWaypoint(i);
      lengthtotal += Task[i].Leg;
    }
  }
  if (lengthtotal>0) {
    for (i=0; i<MAXTASKPOINTS; i++) {
      if (Task[i].Index != -1) {
	RefreshTaskWaypoint(i);
	TaskStats[i].LengthPercent = Task[i].Leg/lengthtotal;
	if ((i<MAXTASKPOINTS-1) && (Task[i+1].Index == -1)) {
	  Task[i].AATTargetOffsetRadius = 0.0;
	  Task[i].AATTargetOffsetRadial = 0.0;
	  Task[i].AATTargetLat = WayPointList[Task[i].Index].Latitude;
	  Task[i].AATTargetLon = WayPointList[Task[i].Index].Longitude;
	}
      }
    }
  }

  // Determine if a waypoint is in the task
  for (i=0; i< (int)NumberOfWayPoints; i++) {
    WayPointList[i].InTask = false;
    if ((WayPointList[i].Flags & HOME) == HOME) {
      WayPointList[i].InTask = true;
    }
  }
  if (HomeWaypoint>=0) {
    WayPointList[HomeWaypoint].InTask = true;
  }
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (Task[i].Index != -1) {
      WayPointList[Task[i].Index].InTask = true;
    }
  }
  if (EnableMultipleStartPoints) {
    for (i=0; i<MAXSTARTPOINTS; i++) {
      if ((StartPoints[i].Index != -1)&&(StartPoints[i].Active)) {
        WayPointList[StartPoints[i].Index].InTask = true;
      }
    }
  }

  CalculateTaskSectors();
  CalculateAATTaskSectors();
}


void CalculateTaskSectors(void)
{
  int i;
  double SectorAngle, SectorSize, SectorBearing;

  if (EnableMultipleStartPoints) {
    for(i=0;i<MAXSTARTPOINTS-1;i++) {
      if (StartPoints[i].Active && (StartPoints[i].Index>=0)) {
	if (StartLine==2) {
          SectorAngle = 45+90;
        } else {
          SectorAngle = 90;
        }
        SectorSize = StartRadius;
        SectorBearing = StartPoints[i].OutBound;

        StartPoints[i].SectorStartLat =
          FindLatitude(WayPointList[StartPoints[i].Index].Latitude,
                       WayPointList[StartPoints[i].Index].Longitude,
                       SectorBearing + SectorAngle, SectorSize);
        StartPoints[i].SectorStartLon =
          FindLongitude(WayPointList[StartPoints[i].Index].Latitude,
                        WayPointList[StartPoints[i].Index].Longitude,
                        SectorBearing + SectorAngle, SectorSize);
        StartPoints[i].SectorEndLat   =
          FindLatitude(WayPointList[StartPoints[i].Index].Latitude,
                       WayPointList[StartPoints[i].Index].Longitude,
                       SectorBearing - SectorAngle, SectorSize);
        StartPoints[i].SectorEndLon   =
          FindLongitude(WayPointList[StartPoints[i].Index].Latitude,
                        WayPointList[StartPoints[i].Index].Longitude,
                        SectorBearing - SectorAngle, SectorSize);
      }
    }
  }

  for(i=0;i<MAXTASKPOINTS-1;i++)
    {
      if((Task[i].Index >=0))
	{
	  if (Task[i+1].Index >=0) {

	    if(i == 0)
	      {
		// start line
		if (StartLine==2) {
		  SectorAngle = 45+90;
		} else {
		  SectorAngle = 90;
		}
		SectorSize = StartRadius;
		SectorBearing = Task[i].OutBound;
	      }
	    else
	      {
		// normal turnpoint sector
		SectorAngle = 45;
		if (SectorType == 2) {
		  SectorSize = 10000; // German DAe 0.5/10
		} else {
		  SectorSize = 5000;  // FAI sector
		}
		SectorBearing = Task[i].Bisector;
	      }
	  } else {
	    // finish line
	    if (FinishLine==2) {
	      SectorAngle = 45;
	    } else {
	      SectorAngle = 90;
	    }
	    SectorSize = FinishRadius;
	    SectorBearing = Task[i].InBound;
	  }

	  Task[i].SectorStartLat =
	    FindLatitude(WayPointList[Task[i].Index].Latitude,
			 WayPointList[Task[i].Index].Longitude,
			 SectorBearing + SectorAngle, SectorSize);
	  Task[i].SectorStartLon =
	    FindLongitude(WayPointList[Task[i].Index].Latitude,
			  WayPointList[Task[i].Index].Longitude,
			  SectorBearing + SectorAngle, SectorSize);
	  Task[i].SectorEndLat   =
	    FindLatitude(WayPointList[Task[i].Index].Latitude,
			 WayPointList[Task[i].Index].Longitude,
			 SectorBearing - SectorAngle, SectorSize);
	  Task[i].SectorEndLon   =
	    FindLongitude(WayPointList[Task[i].Index].Latitude,
			  WayPointList[Task[i].Index].Longitude,
			  SectorBearing - SectorAngle, SectorSize);
	}
    }
}


double AdjustAATTargets(double desired) {
  int i, istart, inum;
  double av=0;
  istart = max(1,ActiveWayPoint);
  inum=0;

  for(i=istart;i<MAXTASKPOINTS-1;i++)
    {
      if((Task[i].Index >=0)&&(Task[i+1].Index >=0))
	{
	  av += Task[i].AATTargetOffsetRadius;
	  inum++;
	}
    }
  if (inum>0) {
    av/= inum;
  }
  if (fabs(desired)>1.0) {
    // don't adjust, just retrieve.
    return av;
  }

  for(i=istart;i<MAXTASKPOINTS-1;i++)
    {
      if((Task[i].Index >=0)&&(Task[i+1].Index >=0))
	{
	  double d = Task[i].AATTargetOffsetRadius;
	  d = (desired-av)*(1.0-d)-1.0;
	  Task[i].AATTargetOffsetRadius = min(1.0,
                             max(desired,-1.0));
	}
    }
  return av;
}


void CalculateAATTaskSectors(void)
{
  int i;

  if(AATEnabled == FALSE)
    return;

  Task[0].AATTargetOffsetRadius = 0.0;
  Task[0].AATTargetOffsetRadial = 0.0;
  if (Task[0].Index>=0) {
    Task[0].AATTargetLat = WayPointList[Task[0].Index].Latitude;
    Task[0].AATTargetLon = WayPointList[Task[0].Index].Longitude;
  }

  for(i=1;i<MAXTASKPOINTS-1;i++)
    {
      if((Task[i].Index >=0))
	{
	  Task[i].AATTargetOffsetRadius = min(1.0,
                             max(Task[i].AATTargetOffsetRadius,-1.0));

	  Task[i].AATTargetOffsetRadial = min(1.0,
                             max(Task[i].AATTargetOffsetRadial,0.0));

	  double targetbearing;
	  double targetrange;

	  targetbearing = Task[i].Bisector;

	  if(Task[i].AATType == SECTOR) {
	    targetrange = ((Task[i].AATTargetOffsetRadius+1.0)/2.0);
	    if (Task[i].AATFinishRadial>Task[i].AATStartRadial) {
	      if (targetbearing>Task[i].AATFinishRadial) {
		targetbearing = targetbearing+180;
		targetrange = 1.0-targetrange;
	      } else if (targetbearing<Task[i].AATStartRadial) {
		targetbearing = targetbearing+180;
		targetrange = 1.0-targetrange;
	      }
	    } else {
	      if (targetbearing<=Task[i].AATFinishRadial) {
		targetbearing = targetbearing+180;
		targetrange = 1.0-targetrange;
	      } else if (targetbearing>=Task[i].AATStartRadial) {
		targetbearing = targetbearing+180;
		targetrange = 1.0-targetrange;
	      }
	    }
	    if (targetbearing>360) {
	      targetbearing-= 360;
	    }
	    targetrange*= Task[i].AATSectorRadius;
	  } else {
	    targetrange = Task[i].AATTargetOffsetRadius
	      *Task[i].AATCircleRadius;
	  }

	  /*
	  if(Task[i].AATType == SECTOR) {
	    double r1 = Task[i].AATStartRadial;
	    double r2 = Task[i].AATFinishRadial;
	    targetbearing = (r2-r1)*Task[i].AATTargetOffsetRadial+r1;
	  } else {
	    targetbearing = 360.0*Task[i].AATTargetOffsetRadial;
	  }
	  */

	  Task[i].AATTargetLat =
	    FindLatitude (WayPointList[Task[i].Index].Latitude,
			  WayPointList[Task[i].Index].Longitude,
			  targetbearing,
			  targetrange);
	  Task[i].AATTargetLon =
	    FindLongitude (WayPointList[Task[i].Index].Latitude,
			   WayPointList[Task[i].Index].Longitude,
			   targetbearing,
			   targetrange);

	  if(Task[i].AATType == SECTOR)
	    {
	      Task[i].AATStartLat =
		FindLatitude (WayPointList[Task[i].Index].Latitude,
			      WayPointList[Task[i].Index].Longitude,
			      Task[i].AATStartRadial ,
			      Task[i].AATSectorRadius );
	      Task[i].AATStartLon =
		FindLongitude(WayPointList[Task[i].Index].Latitude,
			      WayPointList[Task[i].Index].Longitude,
			      Task[i].AATStartRadial ,
			      Task[i].AATSectorRadius );
	      Task[i].AATFinishLat=
		FindLatitude (WayPointList[Task[i].Index].Latitude,
			      WayPointList[Task[i].Index].Longitude,
			      Task[i].AATFinishRadial ,
			      Task[i].AATSectorRadius );
	      Task[i].AATFinishLon=
		FindLongitude(WayPointList[Task[i].Index].Latitude,
			      WayPointList[Task[i].Index].Longitude,
			      Task[i].AATFinishRadial ,
			      Task[i].AATSectorRadius );
	    }
	}
    }
}


////////////

#include "Logger.h"


void guiStartLogger(bool noAsk) {
  int i;
  if (!LoggerActive) {
    if (ReplayLogger::IsEnabled()) {
      if (LoggerActive)
        guiStopLogger(true);
      return;
    }
    TCHAR TaskMessage[1024];
    _tcscpy(TaskMessage,TEXT("Start Logger With Declaration\r\n"));
    for(i=0;i<MAXTASKPOINTS;i++)
      {
	if(Task[i].Index == -1)
	  {
	    if(i==0)
	      _tcscat(TaskMessage,TEXT("None"));

	    Debounce();
	    break;
	  }
	_tcscat(TaskMessage,WayPointList[ Task[i].Index ].Name);
	_tcscat(TaskMessage,TEXT("\r\n"));
      }

    if(noAsk ||
       (MessageBoxX(hWndMapWindow,TaskMessage,TEXT("Start Logger"),
		   MB_YESNO|MB_ICONQUESTION) == IDYES))
      {

	if (LoggerClearFreeSpace()) {
	  LoggerActive = true;

	  StartLogger(strAssetNumber);
	  LoggerHeader();

	  int ntp=0;
	  for(i=0;i<MAXTASKPOINTS;i++)
	    {
	      if(Task[i].Index == -1) {
		break;
	      }
	      ntp++;
	    }
	  StartDeclaration(ntp);
	  for(i=0;i<MAXTASKPOINTS;i++)
	    {
	      if(Task[i].Index == -1) {
		Debounce();
		break;
	      }
	      AddDeclaration(WayPointList[Task[i].Index].Latitude,
			     WayPointList[Task[i].Index].Longitude,
			     WayPointList[Task[i].Index].Name );
	    }
	  EndDeclaration();
	}
      }
    FullScreen();
  }
}


void guiStopLogger(bool noAsk) {
  if (LoggerActive) {
    if(noAsk ||
       (MessageBoxX(hWndMapWindow,gettext(TEXT("Stop Logger")),
		    gettext(TEXT("Stop Logger")),
		    MB_YESNO|MB_ICONQUESTION) == IDYES))
      LoggerActive = false;
    FullScreen();
  }
}


void guiToggleLogger(bool noAsk) {
  if (LoggerActive) {
    guiStopLogger(noAsk);
  } else {
    guiStartLogger(noAsk);
  }
}


//////////////


void RefreshTaskWaypoint(int i) {
  if(i==0)
    {
      Task[i].Leg = 0;
      Task[i].InBound = 0;
    }
  else
    {
      Task[i].Leg = Distance(WayPointList[Task[i].Index].Latitude,
                             WayPointList[Task[i].Index].Longitude,
                             WayPointList[Task[i-1].Index].Latitude,
                             WayPointList[Task[i-1].Index].Longitude);
      Task[i].InBound = Bearing(WayPointList[Task[i-1].Index].Latitude,
				WayPointList[Task[i-1].Index].Longitude,
                                WayPointList[Task[i].Index].Latitude,
				WayPointList[Task[i].Index].Longitude);
      Task[i-1].OutBound = Task[i].InBound;
      Task[i-1].Bisector = BiSector(Task[i-1].InBound,Task[i-1].OutBound);
      if (i==1) {
        if (EnableMultipleStartPoints) {
          for (int j=0; j<MAXSTARTPOINTS; j++) {
            if ((StartPoints[j].Index != -1)&&(StartPoints[j].Active)) {
              StartPoints[j].OutBound =
                Bearing(WayPointList[StartPoints[j].Index].Latitude,
                        WayPointList[StartPoints[j].Index].Longitude,
                        WayPointList[Task[i].Index].Latitude,
                        WayPointList[Task[i].Index].Longitude);
            }
          }
        }
      }
    }
}


void ReadNewTask(HWND hDlg)
{
  int i;
  int TaskSize;
  int WayPointIndex;
  double TaskLength = 0;
  TCHAR szTaskLength[10];
  TCHAR  WaypointID[WAY_POINT_ID_SIZE + 1];

  ActiveWayPoint = -1;

  for(i=0;i<MAXTASKPOINTS;i++)
    {
      Task[i].Index = -1;
    }
  TaskSize = SendDlgItemMessage(hDlg,IDC_TASK,LB_GETCOUNT,0,0);
  for(i=0;i<TaskSize;i++)
    {
      SendDlgItemMessage(hDlg,IDC_TASK,LB_GETTEXT,i,(LPARAM)(LPCTSTR)WaypointID);
      WayPointIndex = SendDlgItemMessage(hDlg,IDC_WAYPOINTS,LB_FINDSTRING,0,(LPARAM)(LPCTSTR)WaypointID);

      if(WayPointIndex == LB_ERR)
        break;
      else
        {
          Task[i].Index = WayPointIndex;

          RefreshTaskWaypoint(i);

          // JMW TODO: do this for next and previous waypoint also

          TaskLength += Task[i].Leg;
        }
    }

  RefreshTask();

  _stprintf(szTaskLength,TEXT("%2.1f"), DISTANCEMODIFY * TaskLength );
  SetDlgItemText(hDlg,IDC_TASKLENGTH,szTaskLength);
  if(Task[0].Index != -1)
    ActiveWayPoint = 0;
}


// loads a new task from scratch.
void LoadNewTask(TCHAR *szFileName)
{
  HANDLE hFile;
  TASK_POINT Temp;
  START_POINT STemp;
  DWORD dwBytesRead;
  int i;
  bool TaskInvalid = false;

  LockTaskData();

  ActiveWayPoint = -1;
  for(i=0;i<MAXTASKPOINTS;i++)
    {
      Task[i].Index = -1;
    }

  hFile = CreateFile(szFileName,GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

  if(hFile!= INVALID_HANDLE_VALUE )
    {
      for(i=0;i<MAXTASKPOINTS;i++)
        {
          if(!ReadFile(hFile,&Temp,sizeof(TASK_POINT),&dwBytesRead, (OVERLAPPED *)NULL))
            {
              TaskInvalid = true;
              break;
            }
	  memcpy(&Task[i],&Temp, sizeof(TASK_POINT));
	  /*Task[i].InBound = Temp.InBound;
	    Task[i].Index = Temp.Index;
	    Task[i].Leg = Temp.Leg;
	    Task[i].OutBound = Temp.OutBound;
	    Task[i].AATCircleRadius = Temp */

          if((Temp.Index >= (int)NumberOfWayPoints)||(Temp.Index<-1)) {
	    TaskInvalid = true;
	  }

        }

      if (!TaskInvalid) {

	if (!ReadFile(hFile,&AATEnabled,sizeof(BOOL),&dwBytesRead,(OVERLAPPED*)NULL))
	  AATEnabled = FALSE;
	if (!ReadFile(hFile,&AATTaskLength,sizeof(double),&dwBytesRead,(OVERLAPPED*)NULL))
	  AATTaskLength = 0;

	// ToDo review by JW

	// 20060521:sgi added additional task parameters
	if (!ReadFile(hFile,&FinishRadius,sizeof(FinishRadius),&dwBytesRead,(OVERLAPPED*)NULL))
	  void(0);  // todo set default values
	if (!ReadFile(hFile,&FinishLine,sizeof(FinishLine),&dwBytesRead,(OVERLAPPED*)NULL))
	  void(0);
	if (!ReadFile(hFile,&StartRadius,sizeof(StartRadius),&dwBytesRead,(OVERLAPPED*)NULL))
	  void(0);
	if (!ReadFile(hFile,&StartLine,sizeof(StartLine),&dwBytesRead,(OVERLAPPED*)NULL))
	  void(0);
	if (!ReadFile(hFile,&SectorType,sizeof(SectorType),&dwBytesRead,(OVERLAPPED*)NULL))
	  void(0);
	if (!ReadFile(hFile,&SectorRadius,sizeof(SectorRadius),&dwBytesRead,(OVERLAPPED*)NULL))
	  void(0);
	if (!ReadFile(hFile,&AutoAdvance,sizeof(AutoAdvance),&dwBytesRead,(OVERLAPPED*)NULL))
	  void(0);

        if (!ReadFile(hFile,&EnableMultipleStartPoints,sizeof(bool),&dwBytesRead,(OVERLAPPED*)NULL))
          void(0);

        for(i=0;i<MAXSTARTPOINTS;i++)
        {
          if(!ReadFile(hFile,&STemp,sizeof(START_POINT),&dwBytesRead, (OVERLAPPED *)NULL))
            {
              TaskInvalid = true;
              break;
            }

          if((STemp.Index < (int)NumberOfWayPoints) && (STemp.Index>-2))
            {
              memcpy(&StartPoints[i],&STemp, sizeof(START_POINT));
            } else {
	    TaskInvalid = true;
	  }
        }

      }
      CloseHandle(hFile);
  } else {
    TaskInvalid = true;
  }

  if (TaskInvalid) {
    for(i=0;i<MAXTASKPOINTS;i++)
      {
	Task[i].Index = -1;
      }
    ActiveWayPoint = -1;
    DefaultTask();
  }

  RefreshTask();

  if(Task[0].Index != -1)
    ActiveWayPoint = 0;

  UnlockTaskData();

}


void SaveTask(TCHAR *szFileName)
{
  HANDLE hFile;
  DWORD dwBytesWritten;

  LockTaskData();

  hFile = CreateFile(szFileName,GENERIC_WRITE,0,(LPSECURITY_ATTRIBUTES)NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

  if(hFile!=INVALID_HANDLE_VALUE )
    {
      WriteFile(hFile,&Task[0],sizeof(TASK_POINT)*MAXTASKPOINTS,&dwBytesWritten,(OVERLAPPED *)NULL);
      WriteFile(hFile,&AATEnabled,sizeof(BOOL),&dwBytesWritten,(OVERLAPPED*)NULL);
      WriteFile(hFile,&AATTaskLength,sizeof(double),&dwBytesWritten,(OVERLAPPED*)NULL);

// ToDo review by JW

      // 20060521:sgi added additional task parameters
      WriteFile(hFile,&FinishRadius,sizeof(FinishRadius),&dwBytesWritten,(OVERLAPPED*)NULL);
      WriteFile(hFile,&FinishLine,sizeof(FinishLine),&dwBytesWritten,(OVERLAPPED*)NULL);
      WriteFile(hFile,&StartRadius,sizeof(StartRadius),&dwBytesWritten,(OVERLAPPED*)NULL);
      WriteFile(hFile,&StartLine,sizeof(StartLine),&dwBytesWritten,(OVERLAPPED*)NULL);
      WriteFile(hFile,&SectorType,sizeof(SectorType),&dwBytesWritten,(OVERLAPPED*)NULL);
      WriteFile(hFile,&SectorRadius,sizeof(SectorRadius),&dwBytesWritten,(OVERLAPPED*)NULL);
      WriteFile(hFile,&AutoAdvance,sizeof(AutoAdvance),&dwBytesWritten,(OVERLAPPED*)NULL);

      WriteFile(hFile,&EnableMultipleStartPoints,sizeof(bool),&dwBytesWritten,(OVERLAPPED*)NULL);
      WriteFile(hFile,&StartPoints[0],sizeof(START_POINT)*MAXSTARTPOINTS,&dwBytesWritten,(OVERLAPPED*)NULL);
      CloseHandle(hFile);
    }
  UnlockTaskData();
}
