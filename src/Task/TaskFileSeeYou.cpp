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
#include "WayPoint/WaypointReaderSeeYou.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/BGAEnhancedOptionZone.hpp"
#include "Task/ObservationZones/BGAFixedCourseZone.hpp"
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

/**
 * Parses the Options parameters line from See You task file for one task
 * @param task_info Updated with Options
 * @param params Input array of parameters preparsed from See You task file
 * @param n_params number parameters in the line
 */
static void
ParseOptions(SeeYouTaskInformation *task_info, const TCHAR *params[],
    const size_t n_params)
{
  // Iterate through available task options
  for (unsigned i = 1; i < n_params; i++) {
    if (_tcsncmp(params[i], _T("WpDis"), 5) == 0) {
      // Parse WpDis option
      if (_tcslen(params[i]) > 6 &&
          _tcsncmp(params[i] + 6, _T("False"), 5) == 0)
        task_info->WpDis = false;
    } else if (_tcsncmp(params[i], _T("TaskTime"), 8) == 0) {
      // Parse TaskTime option
      if (_tcslen(params[i]) > 9)
        task_info->TaskTime = ParseTaskTime(params[i] + 9);
    }
  }
}

/**
 * Parses one ObsZone line from the See You task file
 * @param turnpoint_infos Updated with the OZ info
 * @param params Input array of parameters preparsed from See You task file
 * @param n_params Number parameters in the line
 */
static void
ParseOZs(SeeYouTurnpointInformation turnpoint_infos[], const TCHAR *params[],
    unsigned n_params)
{
  // Read OZ index
  TCHAR* end;
  const int oz_index = _tcstol(params[0] + 8, &end, 10);
  if (params[0] + 8 == end || oz_index >= 30)
    return;

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

/**
 * Parses Options and OZs from See You task file
 * @param reader.  Points to first line of task after task "Waypoint list" line
 * @param task_info Loads this with CU task options info
 * @param turnpoint_infos Loads this with CU task tp info
 */
static void
ParseCUTaskDetails(FileLineReader &reader, SeeYouTaskInformation *task_info,
  SeeYouTurnpointInformation turnpoint_infos[])
{
  // Read options/observation zones
  TCHAR params_buffer[1024];
  const TCHAR *params[20];
  TCHAR *line;
  const unsigned int max_params = sizeof(params) / sizeof(params[0]);
  while ((line = reader.read()) != NULL &&
         line[0] != _T('\"') && line[0] != _T(',')) {
    const size_t n_params = WaypointReaderBase::
        ExtractParameters(line, params_buffer, params, max_params, true);

    if (_tcscmp(params[0], _T("Options")) == 0) {
      // Options line found
      ParseOptions(task_info, params, n_params);

    } else if (_tcsncmp(params[0], _T("ObsZone"), 7) == 0) {
      // Observation zone line found
      if (_tcslen(params[0]) <= 8)
        continue;

      ParseOZs(turnpoint_infos, params, n_params);
    }
  } // end while
}

static bool isKeyhole(const SeeYouTurnpointInformation &turnpoint_infos)
{
  if (fabs(turnpoint_infos.Angle1.value_degrees() - fixed(45)) < fixed(2) &&
      fabs(turnpoint_infos.Radius1 - fixed(10000)) < fixed(2) &&
      fabs(turnpoint_infos.Angle2.value_degrees() - fixed(180)) < fixed(2) &&
      fabs(turnpoint_infos.Radius2 - fixed(500)) < fixed(2))

    return true;
      else
    return false;
}

static bool isBGAFixedCourseZone(const SeeYouTurnpointInformation &turnpoint_infos)
{
  if (fabs(turnpoint_infos.Angle1.value_degrees() - fixed(45)) < fixed(2) &&
      fabs(turnpoint_infos.Radius1 - fixed(20000)) < fixed(2) &&
      fabs(turnpoint_infos.Angle2.value_degrees() - fixed(180)) < fixed(2) &&
      fabs(turnpoint_infos.Radius2 - fixed(500)) < fixed(2))

    return true;
      else
    return false;
}

static bool isBGAEnhancedOptionZone(const SeeYouTurnpointInformation
                                    &turnpoint_infos)
{
  if (fabs(turnpoint_infos.Angle1.value_degrees() - fixed(90)) < fixed(2) &&
      fabs(turnpoint_infos.Radius1 - fixed(10000)) < fixed(2) &&
      fabs(turnpoint_infos.Angle2.value_degrees() - fixed(180)) < fixed(2) &&
      fabs(turnpoint_infos.Radius2 - fixed(500)) < fixed(2))

    return true;
      else
    return false;
}

/**
 * Creates the correct XCSoar OZ type from the See You OZ options for the point
 * Note: there are several rules enforced here related to the combinations
 * and types of Zones supported by XCSoar.  When XCSoar adds more zone types,
 * the logic below will need to be updated.
 * @param turnpoint_infos Contains the See You turnpoint and OZ info
 * @param current point position
 * @param number wps in task
 * @param array of wps for each point in task
 * @param factType The XCSoar factory type
 * @return the XCSoar OZ
 */
static ObservationZonePoint*
CreateOZ(const SeeYouTurnpointInformation &turnpoint_infos,
         unsigned pos, unsigned size, const Waypoint *wps[],
         TaskBehaviour::Factory_t factType)
{
  ObservationZonePoint* oz = NULL;
  const bool isIntermediate = (pos > 0) && (pos < (size - 1));
  const Waypoint *wp = wps[pos];

  if (!turnpoint_infos.Valid)
    return NULL;

  if (factType == TaskBehaviour::FACTORY_RT &&
      isIntermediate && isKeyhole(turnpoint_infos))
    oz = new KeyholeZone(wp->Location);

  else if (factType == TaskBehaviour::FACTORY_RT &&
      isIntermediate && isBGAEnhancedOptionZone(turnpoint_infos))
    oz = new BGAEnhancedOptionZone(wp->Location);

  else if (factType == TaskBehaviour::FACTORY_RT &&
      isIntermediate && isBGAFixedCourseZone(turnpoint_infos))
    oz = new BGAFixedCourseZone(wp->Location);

  else if (!isIntermediate && turnpoint_infos.Line) // special case "Line"
    oz = new LineSectorZone(wp->Location, turnpoint_infos.Radius1);

  // special case "Cylinder"
  else if (fabs(turnpoint_infos.Angle1.value_degrees() - fixed(180)) < fixed(1) )
    oz = new CylinderZone(wp->Location, turnpoint_infos.Radius1);

  else if (factType == TaskBehaviour::FACTORY_RT) {

    // XCSoar does not support fixed sectors for RT
    if (turnpoint_infos.Style == SeeYouTurnpointInformation::Fixed)
      oz = new CylinderZone(wp->Location, turnpoint_infos.Radius1);
    else
      oz = new FAISectorZone(wp->Location, isIntermediate);

  } else if (isIntermediate) { //AAT intermediate point
    Angle A12adj = turnpoint_infos.Angle12.Reciprocal();
    assert(wps[pos + 1]);
    assert(wps[pos - 1]);

    switch (turnpoint_infos.Style) {
    case SeeYouTurnpointInformation::Fixed: {
      A12adj = turnpoint_infos.Angle12.Reciprocal();
      break;
    }
    case SeeYouTurnpointInformation::Symmetrical: {
      const Angle ap = wps[pos - 1]->Location.bearing(wp->Location);
      const Angle an = wps[pos + 1]->Location.bearing(wp->Location);
      A12adj = ap.HalfAngle(an).Reciprocal();
      break;
    }

    case SeeYouTurnpointInformation::ToNextPoint: {
      A12adj = wps[pos + 1]->Location.bearing(wp->Location);
      break;
    }
    case SeeYouTurnpointInformation::ToPreviousPoint: {
      A12adj = wps[pos - 1]->Location.bearing(wp->Location);
      break;
    }
    case SeeYouTurnpointInformation::ToStartPoint: {
      A12adj = wps[0]->Location.bearing(wp->Location);
      break;
    }
    }

    const Angle RadialStart = (A12adj
        - turnpoint_infos.Angle1).as_bearing();
    const Angle RadialEnd = (A12adj
        + turnpoint_infos.Angle1).as_bearing();

    if (turnpoint_infos.Radius2 > fixed_zero &&
        (turnpoint_infos.Angle2.as_bearing().value_degrees()) < fixed_one) {
      oz = new AnnularSectorZone(wp->Location, turnpoint_infos.Radius1,
          RadialStart, RadialEnd, turnpoint_infos.Radius2);
    } else {
      oz = new SectorZone(wp->Location, turnpoint_infos.Radius1,
          RadialStart, RadialEnd);
    }

  } else { // catch-all
    oz = new CylinderZone(wp->Location, turnpoint_infos.Radius1);
  }

  return oz;
}

/**
 * Creates the XCSoar turnpoint from the See You parameters for the point
 * @param pos The position of the point in the XCSoar task
 * @param n_waypoints Number of points in the XCSoar task
 * @param wp The waypoint
 * @param fact The XCSoar factory
 * @param oz The XCSoar OZ for the point
 * @param factType The XCSoar factory type
 * @return The point
 */
static OrderedTaskPoint*
CreatePoint(unsigned pos, unsigned n_waypoints, const Waypoint *wp,
    AbstractTaskFactory& fact, ObservationZonePoint* oz,
    const TaskBehaviour::Factory_t factType)
{
  OrderedTaskPoint *pt = NULL;

  if (pos == 0)
    pt = (oz ? fact.createStart(oz, *wp) : fact.createStart(*wp));

  else if (pos == n_waypoints - 1)
    pt = (oz ? fact.createFinish(oz, *wp) : fact.createFinish(*wp));

  else if (factType == TaskBehaviour::FACTORY_RT)
    pt = (oz ? fact.createAST(oz, *wp) : fact.createIntermediate(*wp));

  else
    pt = (oz ? fact.createAAT(oz, *wp) : fact.createIntermediate(*wp));

  return pt;
}

static TCHAR *
AdvanceReaderToTask(FileLineReader &reader, const unsigned index)
{
  // Skip lines until n-th task
  unsigned count = 0;
  bool in_task_section = false;
  static TCHAR *line;
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
  return line;
}

OrderedTask*
TaskFileSeeYou::GetTask(const Waypoints *waypoints, unsigned index) const
{
  // Read waypoints from the CUP file
  Waypoints file_waypoints;
  {
    WaypointReaderSeeYou waypoint_file(path, 0);
    if (!waypoint_file.Parse(file_waypoints))
      return NULL;
  }
  file_waypoints.optimise();

  // Create FileReader for reading the task
  FileLineReader reader(path);
  if (reader.error())
    return NULL;

  TCHAR *line = AdvanceReaderToTask(reader, index);

  // Read waypoint list
  // e.g. "Club day 4 Racing task","085PRI","083BOJ","170D_K","065SKY","0844YY", "0844YY"
  //       TASK NAME              , TAKEOFF, START  , TP1    , TP2    , FINISH ,  LANDING
  TCHAR waypoints_buffer[1024];
  const TCHAR *wps[30];
  size_t n_waypoints = WaypointReaderBase::
      ExtractParameters(line, waypoints_buffer, wps, 30, true, _T('"')) - 3;

  SeeYouTaskInformation task_info;
  SeeYouTurnpointInformation turnpoint_infos[30];
  const Waypoint *wpsInTask[30];

  ParseCUTaskDetails(reader, &task_info, turnpoint_infos);

  OrderedTask* task = protected_task_manager->task_blank();
  if (task == NULL)
    return NULL;

  task->set_factory(task_info.WpDis ?
                    TaskBehaviour::FACTORY_RT : TaskBehaviour::FACTORY_AAT);
  AbstractTaskFactory& fact = task->get_factory();
  const TaskBehaviour::Factory_t factType = task->get_factory_type();

  if (factType == TaskBehaviour::FACTORY_AAT) {
    OrderedTaskBehaviour beh = task->get_ordered_task_behaviour();
    beh.aat_min_time = task_info.TaskTime;
    task->set_ordered_task_behaviour(beh);
  }

  // mark task waypoints.  Skip takeoff and landing point
  for (unsigned i = 0; i < n_waypoints; i++) {
    const Waypoint* file_wp = file_waypoints.lookup_name(wps[i + 2]);
    if (file_wp == NULL)
      return NULL;

    const Waypoint* wp = waypoints->get_nearest(file_wp->Location);
    if (wp == NULL || wp->Location.distance(file_wp->Location) > fixed(100))
      wp = file_wp;

    wpsInTask[i] = wp;
  }

  //now create TPs and OZs
  for (unsigned i = 0; i < n_waypoints; i++) {

    ObservationZonePoint* oz = CreateOZ(turnpoint_infos[i], i, n_waypoints,
                                        wpsInTask, factType);
    assert(wpsInTask[i]);
    OrderedTaskPoint *pt = CreatePoint(i, n_waypoints, wpsInTask[i],
                                       fact, oz, factType);

    if (pt != NULL)
      fact.append(*pt, false);

    delete pt;
  }
  return task;
}

unsigned
TaskFileSeeYou::Count()
{
  // Reset internal task name memory
  namesuffixes.clear();

  // Open the CUP file
  FileLineReader reader(path);
  if (reader.error())
    return 0;

  unsigned count = 0;
  bool in_task_section = false;
  TCHAR *line;
  while ((line = reader.read()) != NULL) {
    if (in_task_section) {
      // If the line starts with a string or "nothing" followed
      // by a comma it is a new task definition line
      if (line[0] == _T('\"') || line[0] == _T(',')) {
        // If we still have space in the task name list
        if (count < namesuffixes.capacity()) {
          // If the task doesn't have a name inside the file
          if (line[0] == _T(','))
            namesuffixes.append(NULL);
          else {
            // Ignore starting quote (")
            line++;

            // Save pointer to first character
            TCHAR *name = line;
            // Skip characters until next quote (") or end of string
            while (line[0] != _T('\"') && line[0] != _T('\0'))
              line++;

            // Replace quote (") by end of string (null)
            line[0] = _T('\0');

            // Append task name to the list
            if (_tcslen(name) > 0)
              namesuffixes.append(_tcsdup(name));
            else
              namesuffixes.append(NULL);
          }
        }

        // Increase the task counter
        count++;
      }
    } else if (_tcsicmp(line, _T("-----Related Tasks-----")) == 0) {
      // Found the marker -> all following lines are task lines
      in_task_section = true;
    }
  }

  // Return number of tasks found in the CUP file
  return count;
}
