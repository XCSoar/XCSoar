/*
 Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Util/ExtractParameters.hpp"
#include "Util/StringAPI.hxx"
#include "Util/Macros.hpp"
#include "IO/FileLineReader.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Waypoint/WaypointReaderSeeYou.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Engine/Task/Ordered/Points/ASTPoint.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Operation/Operation.hpp"
#include "Units/System.hpp"

#include <stdlib.h>

struct SeeYouTaskInformation {
  /** True = RT, False = AAT */
  bool wp_dis;
  /** AAT task time in seconds */
  double task_time;
  /** MaxAltStart in meters */
  double max_start_altitude;

  SeeYouTaskInformation():
    wp_dis(true), task_time(0), max_start_altitude(0) {}
};

struct SeeYouTurnpointInformation {
  /** CUP file contained info for this OZ */
  bool valid;

  enum Style {
    FIXED,
    SYMMETRICAL,
    TO_NEXT_POINT,
    TO_PREVIOUS_POINT,
    TO_START_POINT,
  } style;

  bool is_line;
  bool reduce;

  double radius1, radius2, max_altitude;
  Angle angle1, angle2, angle12;

  SeeYouTurnpointInformation():
    valid(false), style(SYMMETRICAL), is_line(false), reduce(false),
    radius1(500), radius2(500),
    max_altitude(0),
    angle1(Angle::Zero()),
    angle2(Angle::Zero()),
    angle12(Angle::Zero()) {}
};

static double
ParseTaskTime(const TCHAR* str)
{
  int hh = 0, mm = 0, ss = 0;
  TCHAR* end;
  hh = _tcstol(str, &end, 10);
  if (str != end && _tcslen(str) > 3 && str[2] == _T(':')) {
    mm = _tcstol(str + 3, &end, 10);
    if (str != end && _tcslen(str + 3) > 3 && str[5] == _T(':'))
      ss = _tcstol(str + 6, nullptr, 10);
  }
  return ss + mm * 60 + hh * 3600;
}

static SeeYouTurnpointInformation::Style
ParseStyle(const TCHAR* str)
{
  int style = 1;
  TCHAR* end;
  style = _tcstol(str, &end, 10);
  if (str == end)
    style = 1;

  return (SeeYouTurnpointInformation::Style)style;
}

static Angle
ParseAngle(const TCHAR* str)
{
  int angle = 0;
  TCHAR* end;
  angle = _tcstol(str, &end, 10);
  if (str == end)
    angle = 0;

  return Angle::Degrees(angle);
}

static double
ParseRadius(const TCHAR* str)
{
  int radius = 500;
  TCHAR* end;
  radius = _tcstol(str, &end, 10);
  if (str == end)
    radius = 500;

  return radius;
}

static double
ParseMaxAlt(const TCHAR* str)
{
  double maxalt = 0;
  TCHAR* end;
  maxalt = _tcstod(str, &end);
  if (str == end)
    return 0;

  if (_tcslen(end) >= 2 && end[0] == _T('f') && end[1] == _T('t'))
    maxalt = Units::ToSysUnit(maxalt, Unit::FEET);

  return maxalt;
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
    if (StringIsEqual(params[i], _T("WpDis"), 5)) {
      // Parse WpDis option
      if (_tcslen(params[i]) > 6 &&
          StringIsEqual(params[i] + 6, _T("False"), 5))
        task_info->wp_dis = false;
    } else if (StringIsEqual(params[i], _T("TaskTime"), 8)) {
      // Parse TaskTime option
      if (_tcslen(params[i]) > 9)
        task_info->task_time = ParseTaskTime(params[i] + 9);
    }
  }
}

/**
 * Parses one ObsZone line from the See You task file
 * @param turnpoint_infos Updated with the OZ info
 * @param params Input array of parameters preparsed from See You task file
 * @param n_params Number parameters in the line
 * @return OZ index from CU (0 to n-1) or -1 if no OZ found
 */
static int
ParseOZs(SeeYouTurnpointInformation turnpoint_infos[], const TCHAR *params[],
         unsigned n_params)
{
  // Read OZ index
  TCHAR* end;
  const int oz_index = _tcstol(params[0] + 8, &end, 10);
  if (params[0] + 8 == end || oz_index >= 30)
    return -1;

  turnpoint_infos[oz_index].valid = true;
  // Iterate through available OZ options
  for (unsigned i = 1; i < n_params; i++) {
    const TCHAR *pair = params[i];
    SeeYouTurnpointInformation &tp_info = turnpoint_infos[oz_index];

    if (StringIsEqual(pair, _T("Style"), 5)) {
      if (_tcslen(pair) > 6)
        tp_info.style = ParseStyle(pair + 6);
    } else if (StringIsEqual(pair, _T("R1="), 3)) {
      if (_tcslen(pair) > 3)
        tp_info.radius1 = ParseRadius(pair + 3);
    } else if (StringIsEqual(pair, _T("A1="), 3)) {
      if (_tcslen(pair) > 3)
        tp_info.angle1 = ParseAngle(pair + 3);
    } else if (StringIsEqual(pair, _T("R2="), 3)) {
      if (_tcslen(pair) > 3)
        tp_info.radius2 = ParseRadius(pair + 3);
    } else if (StringIsEqual(pair, _T("A2="), 3)) {
      if (_tcslen(pair) > 3)
        tp_info.angle2 = ParseAngle(pair + 3);
    } else if (StringIsEqual(pair, _T("A12="), 4)) {
      if (_tcslen(pair) > 3)
        tp_info.angle12 = ParseAngle(pair + 4);
    } else if (StringIsEqual(pair, _T("MaxAlt="), 7)) {
      if (_tcslen(pair) > 7)
        tp_info.max_altitude = ParseMaxAlt(pair + 7);
      } else if (StringIsEqual(pair, _T("Line"), 4)) {
      if (_tcslen(pair) > 5 && pair[5] == _T('1'))
        tp_info.is_line = true;
    } else if (StringIsEqual(pair, _T("Reduce"), 6)) {
      if (_tcslen(pair) > 7 && pair[7] == _T('1'))
        tp_info.reduce = true;
    }
  }
  return oz_index;
}

/**
 * Parses Options and OZs from See You task file
 * @param reader.  Points to first line of task after task "Waypoint list" line
 * @param task_info Loads this with CU task options info
 * @param turnpoint_infos Loads this with CU task tp info
 */
static void
ParseCUTaskDetails(TLineReader &reader, SeeYouTaskInformation *task_info,
                   SeeYouTurnpointInformation turnpoint_infos[])
{
  // Read options/observation zones
  TCHAR params_buffer[1024];
  const TCHAR *params[20];
  TCHAR *line;
  int TPIndex = 0;
  const unsigned int max_params = ARRAY_SIZE(params);
  while ((line = reader.ReadLine()) != nullptr &&
         line[0] != _T('\"') && line[0] != _T(',')) {
    const size_t n_params = ExtractParameters(line, params_buffer,
                                              params, max_params, true);

    if (StringIsEqual(params[0], _T("Options"))) {
      // Options line found
      ParseOptions(task_info, params, n_params);

    } else if (StringIsEqual(params[0], _T("ObsZone"), 7)) {
      // Observation zone line found
      if (_tcslen(params[0]) <= 8)
        continue;

      TPIndex = ParseOZs(turnpoint_infos, params, n_params);
      if (TPIndex == 0)
        task_info->max_start_altitude = turnpoint_infos[TPIndex].max_altitude;
    }
  } // end while
}

static bool isKeyhole(const SeeYouTurnpointInformation &turnpoint_infos)
{
  return (fabs(turnpoint_infos.angle1.Degrees() - 45) < 2 &&
          fabs(turnpoint_infos.radius1 - 10000) < 2 &&
          fabs(turnpoint_infos.angle2.Degrees() - 180) < 2 &&
          fabs(turnpoint_infos.radius2 - 500) < 2);
}

static bool isBGAFixedCourseZone(const SeeYouTurnpointInformation &turnpoint_infos)
{
  return (fabs(turnpoint_infos.angle1.Degrees() - 45) < 2 &&
          fabs(turnpoint_infos.radius1 - 20000) < 2 &&
          fabs(turnpoint_infos.angle2.Degrees() - 180) < 2 &&
          fabs(turnpoint_infos.radius2 - 500) < 2);
}

static bool isBGAEnhancedOptionZone(const SeeYouTurnpointInformation
                                    &turnpoint_infos)
{
  return (fabs(turnpoint_infos.angle1.Degrees() - 90) < 2 &&
          fabs(turnpoint_infos.radius1 - 10000) < 2 &&
          fabs(turnpoint_infos.angle2.Degrees() - 180) < 2 &&
          fabs(turnpoint_infos.radius2 - 500) < 2);
}

gcc_pure
static Angle
CalcIntermediateAngle(const SeeYouTurnpointInformation &turnpoint_infos,
                      const GeoPoint &location,
                      const GeoPoint &start,
                      const GeoPoint &previous,
                      const GeoPoint &next)
{
    switch (turnpoint_infos.style) {
    case SeeYouTurnpointInformation::FIXED:
      return turnpoint_infos.angle12.Reciprocal();

    case SeeYouTurnpointInformation::SYMMETRICAL:
      break;

    case SeeYouTurnpointInformation::TO_NEXT_POINT:
      return next.Bearing(location);

    case SeeYouTurnpointInformation::TO_PREVIOUS_POINT:
      return previous.Bearing(location);

    case SeeYouTurnpointInformation::TO_START_POINT:
      return start.Bearing(location);
    }

    /* SYMMETRICAL is the fallback when the file contained an
       invalid/unknown style */
    const Angle ap = previous.Bearing(location);
    const Angle an = next.Bearing(location);
    return ap.HalfAngle(an).Reciprocal();
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
         unsigned pos, unsigned size, const WaypointPtr wps[],
         TaskFactoryType factType)
{
  ObservationZonePoint* oz = nullptr;
  const bool is_intermediate = (pos > 0) && (pos < (size - 1));
  const Waypoint *wp = &*wps[pos];

  if (!turnpoint_infos.valid)
    return nullptr;

  if (factType == TaskFactoryType::RACING &&
      is_intermediate && isKeyhole(turnpoint_infos))
    oz = KeyholeZone::CreateDAeCKeyholeZone(wp->location);

  else if (factType == TaskFactoryType::RACING &&
      is_intermediate && isBGAEnhancedOptionZone(turnpoint_infos))
    oz = KeyholeZone::CreateBGAEnhancedOptionZone(wp->location);

  else if (factType == TaskFactoryType::RACING &&
      is_intermediate && isBGAFixedCourseZone(turnpoint_infos))
    oz = KeyholeZone::CreateBGAFixedCourseZone(wp->location);

  else if (!is_intermediate && turnpoint_infos.is_line) // special case "is_line"
    oz = new LineSectorZone(wp->location, turnpoint_infos.radius1);

  // special case "Cylinder"
  else if (fabs(turnpoint_infos.angle1.Degrees() - 180) < 1 )
    oz = new CylinderZone(wp->location, turnpoint_infos.radius1);

  else if (factType == TaskFactoryType::RACING) {

    // XCSoar does not support fixed sectors for RT
    if (turnpoint_infos.style == SeeYouTurnpointInformation::FIXED)
      oz = new CylinderZone(wp->location, turnpoint_infos.radius1);
    else
      oz = SymmetricSectorZone::CreateFAISectorZone(wp->location,
                                                    is_intermediate);

  } else if (is_intermediate) { //AAT intermediate point
    assert(wps[pos + 1]);
    assert(wps[pos - 1]);

    const Angle A12adj = CalcIntermediateAngle(turnpoint_infos,
                                               wp->location,
                                               wps[pos - 1]->location,
                                               wps[pos + 1]->location,
                                               wps[0]->location);

    const Angle RadialStart = (A12adj - turnpoint_infos.angle1).AsBearing();
    const Angle RadialEnd = (A12adj + turnpoint_infos.angle1).AsBearing();

    if (turnpoint_infos.radius2 > 0 &&
        (turnpoint_infos.angle2.AsBearing().Degrees()) < 1) {
      oz = new AnnularSectorZone(wp->location, turnpoint_infos.radius1,
          RadialStart, RadialEnd, turnpoint_infos.radius2);
    } else {
      oz = new SectorZone(wp->location, turnpoint_infos.radius1,
          RadialStart, RadialEnd);
    }

  } else { // catch-all
    oz = new CylinderZone(wp->location, turnpoint_infos.radius1);
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
CreatePoint(unsigned pos, unsigned n_waypoints, WaypointPtr &&wp,
    AbstractTaskFactory& fact, ObservationZonePoint* oz,
    const TaskFactoryType factType)
{
  OrderedTaskPoint *pt = nullptr;

  if (pos == 0)
    pt = oz
      ? fact.CreateStart(oz, std::move(wp))
      : fact.CreateStart(std::move(wp));

  else if (pos == n_waypoints - 1)
    pt = oz
      ? fact.CreateFinish(oz, std::move(wp))
      : fact.CreateFinish(std::move(wp));

  else if (factType == TaskFactoryType::RACING)
    pt = oz
      ? fact.CreateASTPoint(oz, std::move(wp))
      : fact.CreateIntermediate(std::move(wp));

  else
    pt = oz
      ? fact.CreateAATPoint(oz, std::move(wp))
      : fact.CreateIntermediate(std::move(wp));

  return pt;
}

static TCHAR *
AdvanceReaderToTask(TLineReader &reader, const unsigned index)
{
  // Skip lines until n-th task
  unsigned count = 0;
  bool in_task_section = false;
  TCHAR *line;
  for (unsigned i = 0; (line = reader.ReadLine()) != nullptr; i++) {
    if (in_task_section) {
      if (line[0] == _T('\"') || line[0] == _T(',')) {
        if (count == index)
          break;

        count++;
      }
    } else if (StringIsEqualIgnoreCase(line, _T("-----Related Tasks-----"))) {
      in_task_section = true;
    }
  }
  return line;
}

OrderedTask*
TaskFileSeeYou::GetTask(const TaskBehaviour &task_behaviour,
                        const Waypoints *waypoints, unsigned index) const
try {
  // Create FileReader for reading the task
  FileLineReader reader(path, Charset::AUTO);

  // Read waypoints from the CUP file
  Waypoints file_waypoints;
  {
    const WaypointFactory factory(WaypointOrigin::NONE);
    WaypointReaderSeeYou waypoint_file(factory);
    NullOperationEnvironment operation;
    waypoint_file.Parse(file_waypoints, reader, operation);
  }
  file_waypoints.Optimise();

  reader.Rewind();

  TCHAR *line = AdvanceReaderToTask(reader, index);
  if (line == nullptr)
    return nullptr;

  // Read waypoint list
  // e.g. "Club day 4 Racing task","085PRI","083BOJ","170D_K","065SKY","0844YY", "0844YY"
  //       TASK NAME              , TAKEOFF, START  , TP1    , TP2    , FINISH ,  LANDING
  TCHAR waypoints_buffer[1024];
  const TCHAR *wps[30];
  size_t n_waypoints = ExtractParameters(line, waypoints_buffer, wps, 30,
                                         true, _T('"'));

  // Some versions of StrePla append a trailing ',' without a following
  // WP name resulting an empty last entry. Remove it from the results
  if (n_waypoints > 0 && wps[n_waypoints - 1][0] == _T('\0'))
    n_waypoints --;

  // At least taskname and takeoff, start, finish and landing points are needed
  if (n_waypoints < 5)
    return nullptr;

  // Remove taskname, start point and landing point from count
  n_waypoints -= 3;

  SeeYouTaskInformation task_info;
  SeeYouTurnpointInformation turnpoint_infos[30];
  WaypointPtr waypoints_in_task[30];

  ParseCUTaskDetails(reader, &task_info, turnpoint_infos);

  OrderedTask *task = new OrderedTask(task_behaviour);
  task->SetFactory(task_info.wp_dis ?
                    TaskFactoryType::RACING : TaskFactoryType::AAT);
  AbstractTaskFactory& fact = task->GetFactory();
  const TaskFactoryType factType = task->GetFactoryType();

  OrderedTaskSettings beh = task->GetOrderedTaskSettings();
  if (factType == TaskFactoryType::AAT) {
    beh.aat_min_time = task_info.task_time;
  }
  if (factType == TaskFactoryType::AAT ||
      factType == TaskFactoryType::RACING) {
    beh.start_constraints.max_height = (unsigned)task_info.max_start_altitude;
    beh.start_constraints.max_height_ref = AltitudeReference::MSL;
  }
  task->SetOrderedTaskSettings(beh);

  // mark task waypoints.  Skip takeoff and landing point
  for (unsigned i = 0; i < n_waypoints; i++) {
    auto file_wp = file_waypoints.LookupName(wps[i + 2]);
    if (file_wp == nullptr)
      return nullptr;

    // Try to find waypoint by name
    auto wp = waypoints->LookupName(file_wp->name);

    // If waypoint by name found and closer than 10m to the original
    if (wp != nullptr &&
        wp->location.DistanceS(file_wp->location) <= 10) {
      // Use this waypoint for the task
      waypoints_in_task[i] = wp;
      continue;
    }

    // Try finding the closest waypoint to the original one
    wp = waypoints->GetNearest(file_wp->location, 10);

    // If closest waypoint found and closer than 10m to the original
    if (wp != nullptr &&
        wp->location.DistanceS(file_wp->location) <= 10) {
      // Use this waypoint for the task
      waypoints_in_task[i] = wp;
      continue;
    }

    // Use the original waypoint
    waypoints_in_task[i] = file_wp;
  }

  //now create TPs and OZs
  for (unsigned i = 0; i < n_waypoints; i++) {

    ObservationZonePoint* oz = CreateOZ(turnpoint_infos[i], i, n_waypoints,
                                        waypoints_in_task, factType);
    assert(waypoints_in_task[i]);
    OrderedTaskPoint *pt = CreatePoint(i, n_waypoints,
                                       WaypointPtr(waypoints_in_task[i]),
                                       fact, oz, factType);

    if (pt != nullptr)
      fact.Append(*pt, false);

    delete pt;
  }
  return task;
} catch (const std::runtime_error &e) {
  return nullptr;
}

unsigned
TaskFileSeeYou::Count()
try {
  // Reset internal task name memory
  namesuffixes.clear();

  // Open the CUP file
  FileLineReader reader(path, Charset::AUTO);

  unsigned count = 0;
  bool in_task_section = false;
  TCHAR *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (in_task_section) {
      // If the line starts with a string or "nothing" followed
      // by a comma it is a new task definition line
      if (line[0] == _T('\"') || line[0] == _T(',')) {
        // If we still have space in the task name list
        if (count < namesuffixes.capacity()) {
          // If the task doesn't have a name inside the file
          if (line[0] == _T(','))
            namesuffixes.append(nullptr);
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
              namesuffixes.append(nullptr);
          }
        }

        // Increase the task counter
        count++;
      }
    } else if (StringIsEqualIgnoreCase(line, _T("-----Related Tasks-----"))) {
      // Found the marker -> all following lines are task lines
      in_task_section = true;
    }
  }

  // Return number of tasks found in the CUP file
  return count;
} catch (const std::runtime_error &e) {
  return 0;
}
