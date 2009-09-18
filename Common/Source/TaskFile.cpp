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

#include "TaskFile.hpp"
#include "Task.h"
#include "LocalPath.hpp"
#include "WayPoint.hpp"
#include "Waypointparser.h"
#include "SettingsTask.hpp"
#include "Protection.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "Components.hpp"
#include "WayPointList.hpp"

#include <stdio.h>
#include <assert.h>

TCHAR LastTaskFileName[MAX_PATH]= TEXT("\0");

void Task::ClearTaskFileName() {
  LastTaskFileName[0] = _T('\0');
}

const TCHAR* Task::getTaskFilename() const {
  return LastTaskFileName;
}


static int FindOrAddWaypoint(WAYPOINT *read_waypoint) {
  // this is an invalid pointer!
  assert(read_waypoint != NULL);

  read_waypoint->Details = 0;
  read_waypoint->Name[NAME_SIZE-1] = 0; // prevent overrun if data is bogus

  int waypoint_index = FindMatchingWaypoint(way_points, read_waypoint);
  if (waypoint_index == -1) {
    // waypoint not found, so add it!

    // TODO bug: Set WAYPOINTFILECHANGED so waypoints get saved?

    waypoint_index = way_points.append(*read_waypoint);
    if (waypoint_index < 0) {
      // error, can't allocate!
      return false;
    }
  }
  return waypoint_index;
}


bool Task::LoadTaskWaypoints(FILE *file) {
  WAYPOINT read_waypoint;

  int i;
  for(i=0;i<MAXTASKPOINTS;i++) {
    if (fread(&read_waypoint, sizeof(read_waypoint), 1, file) != 1) {
      return false;
    }
    if (task_points[i].Index != -1) {
      task_points[i].Index = FindOrAddWaypoint(&read_waypoint);
    }
  }
  for(i=0;i<MAXSTARTPOINTS;i++) {
    if (fread(&read_waypoint, sizeof(read_waypoint), 1, file) != 1) {
      return false;
    }
    if (task_start_points[i].Index != -1) {
      task_start_points[i].Index = FindOrAddWaypoint(&read_waypoint);
    }
  }
  // managed to load everything
  return true;
}

#define  BINFILEMAGICNUMBER     0x5cf78fcf

// loads a new task from scratch.
void Task::LoadNewTask(const TCHAR *szFileName,
                       const SETTINGS_COMPUTER &settings_computer)
{
  TASK_POINT Temp;
  START_POINT STemp;
  int i;
  bool TaskInvalid = false;
  bool WaypointInvalid = false;
  bool TaskLoaded = false;
  unsigned magic = 0;

  ActiveTaskPoint = 0;
  for(i=0;i<MAXTASKPOINTS;i++) {
    task_points[i].Index = -1;
  }

  FILE *file = _tfopen(szFileName, _T("rb"));
  if(file != NULL)
    {
      if (fread(&magic, sizeof(magic), 1, file) != 1) {
	TaskInvalid = true;
      } else if (magic != BINFILEMAGICNUMBER) {
	TaskInvalid = true;
      } else {

      // Defaults
      int   old_StartLine    = StartLine;
      int   old_SectorType   = SectorType;
      DWORD old_SectorRadius = SectorRadius;
      DWORD old_StartRadius  = StartRadius;
      int   old_AutoAdvance  = AutoAdvance;
      double old_AATTaskLength = AATTaskLength;
      BOOL   old_AATEnabled  = AATEnabled;
      DWORD  old_FinishRadius = FinishRadius;
      int    old_FinishLine = FinishLine;
      bool   old_EnableMultipleStartPoints = EnableMultipleStartPoints;

      TaskLoaded = true;

      for(i=0;i<MAXTASKPOINTS;i++)
        {
          if (fread(&Temp, sizeof(Temp), 1, file) != 1)
            {
              TaskInvalid = true;
              break;
            }
	  memcpy(&task_points[i],&Temp, sizeof(TASK_POINT));

          if(!way_points.verify_index(Temp.Index) && (Temp.Index != -1)) {
            // Task is only invalid here if the index is out of range
            // of the waypoints and not equal to -1.
            // (Because -1 indicates a null task item)
	    WaypointInvalid = true;
	  }

        }

      if (!TaskInvalid) {

        if (fread(&AATEnabled, sizeof(AATEnabled), 1, file) != 1) {
          TaskInvalid = true;
        }
        if (fread(&AATTaskLength, sizeof(AATTaskLength), 1, file) != 1) {
          TaskInvalid = true;
        }

	// ToDo review by JW

	// 20060521:sgi added additional task parameters
        if (fread(&FinishRadius, sizeof(FinishRadius), 1, file) != 1) {
          TaskInvalid = true;
        }
        if (fread(&FinishLine, sizeof(FinishLine), 1, file) != 1) {
          TaskInvalid = true;
        }
        if (fread(&StartRadius, sizeof(StartRadius), 1, file) != 1) {
          TaskInvalid = true;
        }
        if (fread(&StartLine, sizeof(StartLine), 1, file) != 1) {
          TaskInvalid = true;
        }
        if (fread(&SectorType, sizeof(SectorType), 1, file) != 1) {
          TaskInvalid = true;
        }
        if (fread(&SectorRadius, sizeof(SectorRadius), 1, file) != 1) {
          TaskInvalid = true;
        }
        if (fread(&AutoAdvance, sizeof(AutoAdvance), 1, file) != 1) {
          TaskInvalid = true;
        }

        if (fread(&EnableMultipleStartPoints,
                  sizeof(EnableMultipleStartPoints), 1, file) != 1) {
          TaskInvalid = true;
        }

        for(i=0;i<MAXSTARTPOINTS;i++)
        {
          if (fread(&STemp, sizeof(STemp), 1, file) != 1) {
            TaskInvalid = true;
            break;
          }

          if(way_points.verify_index(STemp.Index) || (STemp.Index==-1)) {
            memcpy(&task_start_points[i],&STemp, sizeof(START_POINT));
          } else {
	    WaypointInvalid = true;
	  }
        }

        //// search for waypoints...
        if (!TaskInvalid) {
          if (!LoadTaskWaypoints(file) && WaypointInvalid) {
            // couldn't lookup the waypoints in the file and we know there are invalid waypoints
            TaskInvalid = true;
          }
        }

      }

      if (TaskInvalid) {
        StartLine = old_StartLine;
        SectorType = old_SectorType;
        SectorRadius = old_SectorRadius;
        StartRadius = old_StartRadius;
        AutoAdvance = old_AutoAdvance;
        AATTaskLength = old_AATTaskLength;
        AATEnabled = old_AATEnabled;
        FinishRadius = old_FinishRadius;
        FinishLine = old_FinishLine;
        EnableMultipleStartPoints = old_EnableMultipleStartPoints;
      }
      }

      fclose(file);

  } else {
    TaskInvalid = true;
  }

  if (TaskInvalid) {
    ClearTask();
  }

  RefreshTask(settings_computer);

  if (TaskInvalid && TaskLoaded) {
    MessageBoxX(
      gettext(TEXT("Error in task file!")),
      gettext(TEXT("Load task")),
      MB_OK|MB_ICONEXCLAMATION);
  } else {
    SetTaskModified(false);
    SetTargetModified(false);
    _tcscpy(LastTaskFileName, szFileName);
  }

}


void Task::SaveTask(const TCHAR *szFileName)
{
  FILE *file = _tfopen(szFileName, _T("wb"));
  if (file != NULL) {
    unsigned magic = BINFILEMAGICNUMBER;
    fwrite(&magic, sizeof(magic), 1, file);
    fwrite(&task_points[0], sizeof(task_points[0]), MAXTASKPOINTS, file);
    fwrite(&AATEnabled, sizeof(AATEnabled), 1, file);
    fwrite(&AATTaskLength, sizeof(AATTaskLength), 1, file);

    // 20060521:sgi added additional task parameters
    fwrite(&FinishRadius, sizeof(FinishRadius), 1, file);
    fwrite(&FinishLine, sizeof(FinishLine), 1, file);
    fwrite(&StartRadius, sizeof(StartRadius), 1, file);
    fwrite(&StartLine, sizeof(StartLine), 1, file);
    fwrite(&SectorType, sizeof(SectorType), 1, file);
    fwrite(&SectorRadius, sizeof(SectorRadius), 1, file);
    fwrite(&AutoAdvance, sizeof(AutoAdvance), 1, file);

    fwrite(&EnableMultipleStartPoints,
           sizeof(EnableMultipleStartPoints), 1, file);
    fwrite(&task_start_points[0], sizeof(task_start_points[0]), MAXSTARTPOINTS, file);

    // JMW added writing of waypoint data, in case it's missing
    int i;
    for(i=0;i<MAXTASKPOINTS;i++) {
      const WAYPOINT &way_point =
        way_points.get(way_points.verify_index(task_points[i].Index)
                       ? task_points[i].Index
                       : 0 /* dummy data */);

      fwrite(&way_point, sizeof(way_point), 1, file);
    }
    for(i=0;i<MAXSTARTPOINTS;i++) {
      const WAYPOINT &way_point =
        way_points.get(way_points.verify_index(task_start_points[i].Index)
                       ? task_start_points[i].Index
                       : 0 /* dummy data */);

      fwrite(&way_point, sizeof(way_point), 1, file);
    }

    fclose(file);
    SetTaskModified(false); // task successfully saved
    SetTargetModified(false);
    _tcscpy(LastTaskFileName, szFileName);

  } else {

    MessageBoxX(
                gettext(TEXT("Error in saving task!")),
                gettext(TEXT("Save task")),
                MB_OK|MB_ICONEXCLAMATION);
  }
}


void Task::SaveDefaultTask(void) {
  if (!task.isTaskAborted()) {
    TCHAR buffer[MAX_PATH];
#ifdef GNAV
    LocalPath(buffer, TEXT("persist/Default.tsk"));
#else
    LocalPath(buffer, TEXT("Default.tsk"));
#endif
    task.SaveTask(buffer);
  }
}
