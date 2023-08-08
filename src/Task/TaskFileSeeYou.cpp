// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Task/TaskFileSeeYou.hpp"
#include "util/ExtractParameters.hpp"
#include "util/StringAPI.hxx"
#include "util/Macros.hpp"
#include "io/FileLineReader.hpp"
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
#include "Units/System.hpp"
#include "time/BrokenTime.hpp"

#include <stdlib.h>

static constexpr std::size_t CUP_MAX_TPS = 30;

struct SeeYouTaskInformation {
  /** True = RT, False = AAT */
  bool wp_dis = true;
  /** AAT task time in seconds */
  std::chrono::duration<unsigned> task_time{};
  /** MaxAltStart in meters */
  double max_start_altitude = 0;
};

struct SeeYouTurnpointInformation {
  /** CUP file contained info for this OZ */
  bool valid = false;

  enum Style {
    FIXED,
    SYMMETRICAL,
    TO_NEXT_POINT,
    TO_PREVIOUS_POINT,
    TO_START_POINT,
  } style = SYMMETRICAL;

  bool is_line = false;
  bool reduce = false;

  double radius1 = 500, radius2 = 200, max_altitude = 0;
  Angle angle1{}, angle2{}, angle12{};
};

static std::chrono::duration<unsigned>
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
  return BrokenTime(hh, mm, ss).DurationSinceMidnight();
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
    if (auto wp_dis = StringAfterPrefix(params[i], _T("WpDis="))) {
      // Parse WpDis option
      if (StringIsEqual(wp_dis, _T("False")))
        task_info->wp_dis = false;
    } else if (auto task_time = StringAfterPrefix(params[i], _T("TaskTime="))) {
      // Parse TaskTime option
      if (!StringIsEmpty(task_time))
        task_info->task_time = ParseTaskTime(task_time);
    }
  }
}

/**
 * @param params Input array of parameters preparsed from See You task file
 * @param n_params Number parameters in the line
 */
static void
ParseOZs(SeeYouTurnpointInformation &tp_info, const TCHAR *params[],
         unsigned n_params)
{
  tp_info.valid = true;
  // Iterate through available OZ options
  for (unsigned i = 0; i < n_params; i++) {
    const TCHAR *pair = params[i];

    if (auto style = StringAfterPrefix(pair, _T("Style="))) {
      if (!StringIsEmpty(style))
        tp_info.style = ParseStyle(style);
    } else if (auto r1 = StringAfterPrefix(pair, _T("R1="))) {
      if (!StringIsEmpty(r1))
        tp_info.radius1 = ParseRadius(r1);
    } else if (auto a1 = StringAfterPrefix(pair, _T("A1="))) {
      if (!StringIsEmpty(a1))
        tp_info.angle1 = ParseAngle(a1);
    } else if (auto r2 = StringAfterPrefix(pair, _T("R2="))) {
      if (!StringIsEmpty(r2))
        tp_info.radius2 = ParseRadius(r2);
    } else if (auto a2 = StringAfterPrefix(pair, _T("A2="))) {
      if (!StringIsEmpty(a2))
        tp_info.angle2 = ParseAngle(a2);
    } else if (auto a12 = StringAfterPrefix(pair, _T("A12="))) {
      if (!StringIsEmpty(a12))
        tp_info.angle12 = ParseAngle(a12);
    } else if (auto max_altitude = StringAfterPrefix(pair, _T("MaxAlt="))) {
      if (!StringIsEmpty(max_altitude))
        tp_info.max_altitude = ParseMaxAlt(max_altitude);
    } else if (auto line = StringAfterPrefix(pair, _T("Line="))) {
      if (*line == _T('1'))
        tp_info.is_line = true;
    } else if (auto reduce = StringAfterPrefix(pair, _T("Reduce="))) {
      if (*reduce == _T('1'))
        tp_info.reduce = true;
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
ParseCUTaskDetails(TLineReader &reader, SeeYouTaskInformation *task_info,
                   SeeYouTurnpointInformation turnpoint_infos[])
{
  // Read options/observation zones
  TCHAR params_buffer[1024];
  const TCHAR *params[20];
  TCHAR *line;
  const unsigned int max_params = ARRAY_SIZE(params);
  while ((line = reader.ReadLine()) != nullptr &&
         line[0] != _T('\"') && line[0] != _T(',')) {
    const size_t n_params = ExtractParameters(line, params_buffer,
                                              params, max_params, true);

    if (StringIsEqual(params[0], _T("Options"))) {
      // Options line found
      ParseOptions(task_info, params, n_params);

    } else if (auto obs_zone = StringAfterPrefix(params[0], _T("ObsZone="))) {
      // Observation zone line found
      if (StringIsEmpty(obs_zone))
        continue;

      TCHAR *end;
      const std::size_t TPIndex = _tcstol(obs_zone, &end, 10);
      if (end == obs_zone || TPIndex >= CUP_MAX_TPS)
        continue;

      ParseOZs(turnpoint_infos[TPIndex], params + 1, n_params - 1);
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

[[gnu::pure]]
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
static std::unique_ptr<ObservationZonePoint>
CreateOZ(const SeeYouTurnpointInformation &turnpoint_infos,
         unsigned pos, unsigned size, const WaypointPtr wps[],
         TaskFactoryType factType)
{
  const bool is_intermediate = (pos > 0) && (pos < (size - 1));
  const Waypoint *wp = &*wps[pos];

  if (!turnpoint_infos.valid)
    return nullptr;

  if (factType == TaskFactoryType::RACING &&
      is_intermediate && isKeyhole(turnpoint_infos)) {
    auto oz = KeyholeZone::CreateCustomKeyholeZone(wp->location,
                                                   turnpoint_infos.radius1,
                                                   turnpoint_infos.angle1);
    oz->SetInnerRadius(turnpoint_infos.radius2);
    return oz;
  }

  else if (factType == TaskFactoryType::RACING &&
      is_intermediate && isBGAEnhancedOptionZone(turnpoint_infos))
    return KeyholeZone::CreateBGAEnhancedOptionZone(wp->location);

  else if (factType == TaskFactoryType::RACING &&
      is_intermediate && isBGAFixedCourseZone(turnpoint_infos))
    return KeyholeZone::CreateBGAFixedCourseZone(wp->location);

  else if (!is_intermediate && turnpoint_infos.is_line) // special case "is_line"
    return std::make_unique<LineSectorZone>(wp->location,
                                            turnpoint_infos.radius1);

  // special case "Cylinder"
  else if (fabs(turnpoint_infos.angle1.Degrees() - 180) < 1 )
    return std::make_unique<CylinderZone>(wp->location,
                                          turnpoint_infos.radius1);

  else if (factType == TaskFactoryType::RACING) {

    // XCSoar does not support fixed sectors for RT
    if (turnpoint_infos.style == SeeYouTurnpointInformation::FIXED)
      return std::make_unique<CylinderZone>(wp->location,
                                            turnpoint_infos.radius1);
    else
      return SymmetricSectorZone::CreateFAISectorZone(wp->location,
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
      return std::make_unique<AnnularSectorZone>(wp->location,
                                                 turnpoint_infos.radius1,
                                                 RadialStart, RadialEnd,
                                                 turnpoint_infos.radius2);
    } else {
      return std::make_unique<SectorZone>(wp->location, turnpoint_infos.radius1,
                                          RadialStart, RadialEnd);
    }

  } else { // catch-all
    return std::make_unique<CylinderZone>(wp->location,turnpoint_infos.radius1);
  }
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
static std::unique_ptr<OrderedTaskPoint>
CreatePoint(unsigned pos, unsigned n_waypoints, WaypointPtr &&wp,
            AbstractTaskFactory& fact,
            std::unique_ptr<ObservationZonePoint> oz,
            const TaskFactoryType factType) noexcept
{
  std::unique_ptr<OrderedTaskPoint> pt;

  if (pos == 0)
    pt = oz
      ? fact.CreateStart(std::move(oz), std::move(wp))
      : fact.CreateStart(std::move(wp));

  else if (pos == n_waypoints - 1)
    pt = oz
      ? fact.CreateFinish(std::move(oz), std::move(wp))
      : fact.CreateFinish(std::move(wp));

  else if (factType == TaskFactoryType::RACING)
    pt = oz
      ? fact.CreateASTPoint(std::move(oz), std::move(wp))
      : fact.CreateIntermediate(std::move(wp));

  else
    pt = oz
      ? fact.CreateAATPoint(std::move(oz), std::move(wp))
      : fact.CreateIntermediate(std::move(wp));

  return pt;
}

/**
 * @return true if the "Related Tasks" line was found, false if the
 * file contains no task
 */
static bool
ParseSeeYouWaypoints(TLineReader &reader, Waypoints &way_points)
{
  const WaypointFactory factory(WaypointOrigin::NONE);
  WaypointReaderSeeYou waypoint_file(factory);

  while (true) {
    TCHAR *line = reader.ReadLine();
    if (line == nullptr)
      return false;

    if (StringIsEqualIgnoreCase(line, _T("-----Related Tasks-----")))
      return true;

    waypoint_file.ParseLine(line, way_points);
  }
}

static TCHAR *
AdvanceReaderToTask(TLineReader &reader, const unsigned index)
{
  // Skip lines until n-th task
  unsigned count = 0;
  TCHAR *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (line[0] == _T('\"') || line[0] == _T(',')) {
      if (count == index)
        break;

      count++;
    }
  }
  return line;
}

std::unique_ptr<OrderedTask>
TaskFileSeeYou::GetTask(const TaskBehaviour &task_behaviour,
                        const Waypoints *waypoints, unsigned index) const
try {
  // Create FileReader for reading the task
  FileLineReader reader(path, Charset::AUTO);

  // Read waypoints from the CUP file
  Waypoints file_waypoints;
  if (!ParseSeeYouWaypoints(reader, file_waypoints))
    return nullptr;

  file_waypoints.Optimise();

  TCHAR *line = AdvanceReaderToTask(reader, index);
  if (line == nullptr)
    return nullptr;

  // Read waypoint list
  // e.g. "Club day 4 Racing task","085PRI","083BOJ","170D_K","065SKY","0844YY", "0844YY"
  //       TASK NAME              , TAKEOFF, START  , TP1    , TP2    , FINISH ,  LANDING
  TCHAR waypoints_buffer[1024];
  const TCHAR *wps[CUP_MAX_TPS];
  size_t n_waypoints = ExtractParameters(line, waypoints_buffer, wps, CUP_MAX_TPS,
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
  SeeYouTurnpointInformation turnpoint_infos[CUP_MAX_TPS];
  WaypointPtr waypoints_in_task[CUP_MAX_TPS];

  ParseCUTaskDetails(reader, &task_info, turnpoint_infos);

  auto task = std::make_unique<OrderedTask>(task_behaviour);
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

    if (waypoints != nullptr) {
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
    }

    // Use the original waypoint
    waypoints_in_task[i] = file_wp;
  }

  //now create TPs and OZs
  for (unsigned i = 0; i < n_waypoints; i++) {

    auto oz = CreateOZ(turnpoint_infos[i], i, n_waypoints,
                       waypoints_in_task, factType);
    assert(waypoints_in_task[i]);
    auto pt = CreatePoint(i, n_waypoints,
                          WaypointPtr(waypoints_in_task[i]),
                          fact, std::move(oz), factType);

    if (pt != nullptr)
      fact.Append(*pt, false);
  }
  return task;
} catch (...) {
  return nullptr;
}

std::vector<tstring>
TaskFileSeeYou::GetList() const
{
  std::vector<tstring> result;

  // Open the CUP file
  FileLineReader reader(path, Charset::AUTO);

  bool in_task_section = false;
  TCHAR *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (in_task_section) {
      // If the line starts with a string or "nothing" followed
      // by a comma it is a new task definition line
      if (line[0] == _T('\"') || line[0] == _T(',')) {
        // If the task doesn't have a name inside the file
        if (line[0] == _T(','))
          result.emplace_back();
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
          result.emplace_back(name);
        }
      }
    } else if (StringIsEqualIgnoreCase(line, _T("-----Related Tasks-----"))) {
      // Found the marker -> all following lines are task lines
      in_task_section = true;
    }
  }

  return result;
}
