/*
 Copyright_License {

 XCSoar Glide Computer - http://www.xcsoar.org/
 Copyright (C) 2000-2011 The XCSoar Project
 A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "Task/TaskFileSeeYou.hpp"

#include "IO/FileLineReader.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "WayPoint/WayPointFileSeeYou.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"

struct SeeYouTaskInformation {
  /** True = RT, False = AAT */
  bool WpDis;
  /** AAT task time in seconds */
  fixed TaskTime;

  SeeYouTaskInformation():
    WpDis(true), TaskTime(fixed_zero) {}
};

struct SeeYouTurnpointInformation {
  bool Valid; // CUP file contained info for this OZ
  enum style_t {
    Fixed,
    Symmetrical,
    ToNextPoint,
    ToPreviousPoint,
    ToStartPoint,
  } Style;

  bool Line;
  bool Reduce;

  fixed Radius1, Radius2;
  Angle Angle1, Angle2, Angle12;

  SeeYouTurnpointInformation():
    Valid(false), Style(Symmetrical), Line(false), Reduce(false),
    Radius1(fixed(500)), Radius2(fixed(500)),
    Angle1(Angle::degrees(fixed_zero)),
    Angle2(Angle::degrees(fixed_zero)),
    Angle12(Angle::degrees(fixed_zero)) {}
};

static fixed
ParseTaskTime(const TCHAR* str)
{
  int hh = 0, mm = 0, ss = 0;
  TCHAR* end;
  hh = _tcstol(str, &end, 10);
  if (str != end && _tcslen(str) > 3 && str[2] == _T(':')) {
    mm = _tcstol(str + 3, &end, 10);
    if (str != end && _tcslen(str + 3) > 3 && str[5] == _T(':'))
      ss = _tcstol(str + 6, NULL, 10);
  }
  return fixed(ss + mm * 60 + hh * 3600);
}

static SeeYouTurnpointInformation::style_t
ParseStyle(const TCHAR* str)
{
  int style = 1;
  TCHAR* end;
  style = _tcstol(str, &end, 10);
  if (str == end)
    style = 1;

  return (SeeYouTurnpointInformation::style_t)style;
}

static Angle
ParseAngle(const TCHAR* str)
{
  int angle = 0;
  TCHAR* end;
  angle = _tcstol(str, &end, 10);
  if (str == end)
    angle = 0;

  return Angle::degrees(fixed(angle));
}

static fixed
ParseRadius(const TCHAR* str)
{
  int radius = 500;
  TCHAR* end;
  radius = _tcstol(str, &end, 10);
  if (str == end)
    radius = 500;

  return fixed(radius);
}

OrderedTask*
TaskFileSeeYou::GetTask(const Waypoints *waypoints, unsigned index) const
{
  // Read waypoints from the CUP file
  Waypoints file_waypoints;
  {
    WayPointFileSeeYou waypoint_file(path, 0);
    if (!waypoint_file.Parse(file_waypoints, NULL))
      return NULL;
  }
  file_waypoints.optimise();

  // Create FileReader for reading the task
  FileLineReader reader(path);
  if (reader.error())
    return NULL;

  // Skip lines until n-th task
  unsigned count = 0;
  bool in_task_section = false;
  TCHAR *line;
  for (unsigned i = 0; (line = reader.read()) != NULL; i++) {
    if (in_task_section) {
      if (line[0] == _T('\"') || line[0] == _T(',')) {
        if (count == index)
          break;

        count++;
      }
    } else if (_tcsicmp(line, _T("-----Related Tasks-----")) == 0) {
      in_task_section = true;
    }
  }

  // Read waypoint list
  // e.g. "Club day 4 Racing task","085PRI","083BOJ","170D_K","065SKY","0844YY", "0844YY"
  //       TASK NAME              , TAKEOFF, START  , TP1    , TP2    , FINISH ,  LANDING
  TCHAR waypoints_buffer[1024];
  const TCHAR *wps[30];
  size_t n_waypoints = WayPointFile::
      extractParameters(line, waypoints_buffer, wps, 30, true, _T('"')) - 3;

  SeeYouTaskInformation task_info;
  SeeYouTurnpointInformation turnpoint_infos[30];

  // Read options/observation zones
  TCHAR params_buffer[1024];
  const TCHAR *params[20];
  const unsigned int max_params = sizeof(params) / sizeof(params[0]);
  while ((line = reader.read()) != NULL &&
         line[0] != _T('\"') && line[0] != _T(',')) {
    size_t n_params = WayPointFile::
        extractParameters(line, params_buffer, params, max_params, true);

    if (_tcscmp(params[0], _T("Options")) == 0) {
      // Options line found
      // Iterate through available task options
      for (unsigned i = 1; i < n_params; i++) {
        if (_tcsncmp(params[i], _T("WpDis"), 5) == 0) {
          // Parse WpDis option
          if (_tcslen(params[i]) > 6 &&
              _tcsncmp(params[i] + 6, _T("False"), 5) == 0)
            task_info.WpDis = false;
        } else if (_tcsncmp(params[i], _T("TaskTime"), 8) == 0) {
          // Parse TaskTime option
          if (_tcslen(params[i]) > 9)
            task_info.TaskTime = ParseTaskTime(params[i] + 9);
        }
      }
    } else if (_tcsncmp(params[0], _T("ObsZone"), 7) == 0) {
      // Observation zone line found

      if (_tcslen(params[0]) <= 8)
        continue;

      // Read OZ index
      TCHAR* end;
      const int oz_index = _tcstol(params[0] + 8, &end, 10);
      if (params[0] + 8 == end || oz_index >= 30)
        continue;

      turnpoint_infos[oz_index].Valid = true;
      // Iterate through available OZ options
      for (unsigned i = 1; i < n_params; i++) {
        if (_tcsncmp(params[i], _T("Style"), 5) == 0) {
          if (_tcslen(params[i]) > 6)
            turnpoint_infos[oz_index].Style = ParseStyle(params[i] + 6);
        } else if (_tcsncmp(params[i], _T("R1="), 3) == 0) {
          if (_tcslen(params[i]) > 3)
            turnpoint_infos[oz_index].Radius1 = ParseRadius(params[i] + 3);
        } else if (_tcsncmp(params[i], _T("A1="), 3) == 0) {
          if (_tcslen(params[i]) > 3)
            turnpoint_infos[oz_index].Angle1 = ParseAngle(params[i] + 3);
        } else if (_tcsncmp(params[i], _T("R2="), 3) == 0) {
          if (_tcslen(params[i]) > 3)
            turnpoint_infos[oz_index].Radius2 = ParseRadius(params[i] + 3);
        } else if (_tcsncmp(params[i], _T("A2="), 3) == 0) {
          if (_tcslen(params[i]) > 3)
            turnpoint_infos[oz_index].Angle2 = ParseAngle(params[i] + 3);
        } else if (_tcsncmp(params[i], _T("A12="), 4) == 0) {
          if (_tcslen(params[i]) > 3)
            turnpoint_infos[oz_index].Angle12 = ParseAngle(params[i] + 4);
        } else if (_tcsncmp(params[i], _T("Line"), 4) == 0) {
          if (_tcslen(params[i]) > 5 && params[i][5] == _T('1'))
            turnpoint_infos[oz_index].Line = true;
        } else if (_tcsncmp(params[i], _T("Reduce"), 6) == 0) {
          if (_tcslen(params[i]) > 7 && params[i][7] == _T('1'))
            turnpoint_infos[oz_index].Reduce = true;
        }
      }
    }
  }

  OrderedTask* task = protected_task_manager->task_blank();
  if (task == NULL)
    return NULL;

  task->set_factory(task_info.WpDis ?
                    TaskBehaviour::FACTORY_RT : TaskBehaviour::FACTORY_AAT);
  AbstractTaskFactory& fact = task->get_factory();

  if (task->get_factory_type() == TaskBehaviour::FACTORY_AAT) {
    OrderedTaskBehaviour beh = task->get_ordered_task_behaviour();
    beh.aat_min_time = task_info.TaskTime;
    task->set_ordered_task_behaviour(beh);
  }

  // load task waypoints.  Skip takeoff and landing point
  for (unsigned i = 0; i < n_waypoints; i++) {
    const Waypoint* file_wp = file_waypoints.lookup_name(wps[i + 2]);
    if (file_wp == NULL)
      return NULL;

    const Waypoint* wp = waypoints->lookup_location(file_wp->Location, fixed(100));
    if (wp == NULL)
      wp = file_wp;

    ObservationZonePoint* oz;
    if (!turnpoint_infos[i].Valid)
      oz = NULL;

    else if (turnpoint_infos[i].Line)
      oz = new LineSectorZone(wp->Location, turnpoint_infos[i].Radius1);

    else if (fabs(turnpoint_infos[i].Angle1.value_degrees() - fixed(180)) < fixed(1) )
      oz = new CylinderZone(wp->Location, turnpoint_infos[i].Radius1);

    else if (turnpoint_infos[i].Style == SeeYouTurnpointInformation::Fixed)
      oz = new SectorZone(wp->Location, turnpoint_infos[i].Radius1,
                          turnpoint_infos[i].Angle1, turnpoint_infos[i].Angle2);
    // symmetric sector
    else if (task->get_factory_type() == TaskBehaviour::FACTORY_RT)
      oz = new FAISectorZone(wp->Location, (i > 0 && (i < n_waypoints - 1)));

    else // XCSoar does not support symmetric sector for AAT
      oz = new CylinderZone(wp->Location, turnpoint_infos[i].Radius1);

    OrderedTaskPoint *pt = NULL;
    if (i == 0  )
      pt = (oz ? fact.createStart(oz, *wp) : fact.createStart(*wp));

    else if (i == n_waypoints - 1)
      pt = (oz ? fact.createFinish(oz, *wp) : fact.createFinish(*wp));

    else if (task->get_factory_type() == TaskBehaviour::FACTORY_RT)
      pt = (oz ? fact.createAST(oz, *wp) : fact.createIntermediate(*wp));

    else
      pt = (oz ? fact.createAAT(oz, *wp) : fact.createIntermediate(*wp));

    if (pt != NULL)
      fact.append(*pt, false);

    delete pt;
  }

  return task;
}

unsigned
TaskFileSeeYou::Count()
{
  FileLineReader reader(path);
  if (reader.error())
    return 0;

  unsigned count = 0;
  bool in_task_section = false;
  TCHAR *line;
  for (unsigned i = 0; (line = reader.read()) != NULL; i++) {
    if (in_task_section) {
      if (line[0] == _T('\"')) {

        if (count < namesuffixes.MAX_SIZE) {
          unsigned cc = 0;
          TCHAR buff[40];
          for (cc = 1; line[cc] && line[cc] != '"' && cc < 40; cc++) {
            buff[cc - 1] = line[cc];
          }
          buff[--cc] = '\0';
          if (_tcslen(buff) > 0) {
            namesuffixes.append(_tcsdup(buff));
          } else {
            namesuffixes.append(NULL);
          }
        }
        count++;
      }
    } else if (_tcsicmp(line, _T("-----Related Tasks-----")) == 0) {
      in_task_section = true;
    }
  }

  return count;
}
