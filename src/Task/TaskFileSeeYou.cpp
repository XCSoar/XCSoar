// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Task/TaskFileSeeYou.hpp"
#include "io/BufferedReader.hxx"
#include "io/BufferedCsvReader.hpp"
#include "io/FileReader.hxx"
#include "io/StringConverter.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Waypoint/CupParser.hpp"
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
#include "util/DecimalParser.hxx"
#include "util/IterableSplitString.hxx"
#include "util/NumberParser.hxx"

#include <stdlib.h>

using std::string_view_literals::operator""sv;

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

[[gnu::pure]]
static std::chrono::duration<unsigned>
ParseTaskTime(std::string_view src) noexcept
{
  unsigned hh, mm = 0, ss = 0;

  if (src.size() < 2)
    return {};

  if (!ParseIntegerTo(src.substr(0, 2), hh))
    return {};

  if (src.size() > 2) {
    if (src[2] != ':' || src.size() < 5)
      return {};

    if (!ParseIntegerTo(src.substr(3, 2), mm))
      return {};

    if (src.size() > 5) {
      if (src[5] != ':' || src.size() != 8)
        return {};

      if (!ParseIntegerTo(src.substr(6, 2), ss))
        return {};
    }
  }

  return BrokenTime(hh, mm, ss).DurationSinceMidnight();
}

[[gnu::pure]]
static SeeYouTurnpointInformation::Style
ParseStyle(std::string_view src) noexcept
{
  if (auto value = ParseInteger<unsigned>(src))
    return static_cast<SeeYouTurnpointInformation::Style>(*value);

  return SeeYouTurnpointInformation::Style::SYMMETRICAL;
}

[[gnu::pure]]
static Angle
ParseAngle(std::string_view src) noexcept
{
  if (auto value = ParseInteger<unsigned>(src))
    return Angle::Degrees(*value);

  return Angle::Zero();
}

[[gnu::pure]]
static double
ParseRadius(std::string_view src) noexcept
{
  if (src.ends_with('m'))
    src.remove_suffix(1);

  if (auto value = ParseInteger<unsigned>(src))
    return *value;

  return 500;
}

[[gnu::pure]]
static double
ParseMaxAlt(std::string_view src) noexcept
{
  Unit unit = Unit::METER;

  if (RemoveSuffix(src, "ft"sv))
    unit = Unit::FEET;
  else
    RemoveSuffix(src, "m"sv);

  if (auto value = ParseDecimal(src))
    return Units::ToSysUnit(*value, unit);

  return 0;
}

/**
 * Parses the Options parameters line from See You task file for one task
 * @param task_info Updated with Options
 * @param params Input array of parameters preparsed from See You task file
 * @param n_params number parameters in the line
 */
static void
ParseOptions(SeeYouTaskInformation &task_info, std::string_view src) noexcept
{
  for (std::string_view i : IterableSplitString(src, ',')) {
    if (SkipPrefix(i, "WpDis="sv)) {
      if (i == "False"sv)
        task_info.wp_dis = false;
    } else if (SkipPrefix(i, "TaskTime="sv)) {
      task_info.task_time = ParseTaskTime(i);
    }
  }
}

/**
 * @param params Input array of parameters preparsed from See You task file
 * @param n_params Number parameters in the line
 */
static void
ParseOZs(SeeYouTurnpointInformation &tp_info, std::string_view src) noexcept
{
  tp_info.valid = true;

  for (std::string_view i : IterableSplitString(src, ',')) {
    if (SkipPrefix(i, "Style="sv))
      tp_info.style = ParseStyle(i);
    else if (SkipPrefix(i, "R1="sv))
      tp_info.radius1 = ParseRadius(i);
    else if (SkipPrefix(i, "A1="sv))
      tp_info.angle1 = ParseAngle(i);
    else if (SkipPrefix(i, "R2="sv))
      tp_info.radius2 = ParseRadius(i);
    else if (SkipPrefix(i, "A2="sv))
      tp_info.angle2 = ParseAngle(i);
    else if (SkipPrefix(i, "A12="sv))
      tp_info.angle12 = ParseAngle(i);
    else if (SkipPrefix(i, "MaxAlt="sv))
      tp_info.max_altitude = ParseMaxAlt(i);
    else if (SkipPrefix(i, "Line="sv))
      tp_info.is_line = i.starts_with('1');
    else if (SkipPrefix(i, "Reduce="sv))
      tp_info.reduce = i.starts_with('1');
  }
}

/**
 * Parses Options and OZs from See You task file
 * @param reader.  Points to first line of task after task "Waypoint list" line
 * @param task_info Loads this with CU task options info
 * @param turnpoint_infos Loads this with CU task tp info
 */
static void
ParseCUTaskDetails(BufferedReader &reader, SeeYouTaskInformation &task_info,
                   SeeYouTurnpointInformation turnpoint_infos[])
{
  // Read options/observation zones
  char *line;
  while ((line = reader.ReadLine()) != nullptr &&
         line[0] != '\"' && line[0] != ',') {
    std::string_view src{line};

    if (SkipPrefix(src, "Options,"sv)) {
      ParseOptions(task_info, src);

    } else if (SkipPrefix(src, "ObsZone="sv)) {
      auto [index_string, rest] = Split(src, ',');

      std::size_t index;
      if (!ParseIntegerTo(index_string, index))
        continue;

      if (index >= CUP_MAX_TPS)
        continue;

      ParseOZs(turnpoint_infos[index], rest);
      if (index == 0)
        task_info.max_start_altitude = turnpoint_infos[index].max_altitude;
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
ParseSeeYouWaypoints(BufferedReader &reader, Waypoints &way_points)
{
  const WaypointFactory factory(WaypointOrigin::NONE);

  return ParseSeeYou(factory, way_points, reader);
}

static char *
AdvanceReaderToTask(BufferedReader &reader, const unsigned index)
{
  // Skip lines until n-th task
  unsigned count = 0;
  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (line[0] == '\"' || line[0] == ',') {
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
  FileReader file_reader{path};
  BufferedReader reader{file_reader};
  StringConverter string_converter;

  // Read waypoints from the CUP file
  Waypoints file_waypoints;
  if (!ParseSeeYouWaypoints(reader, file_waypoints))
    return nullptr;

  file_waypoints.Optimise();

  char *line = AdvanceReaderToTask(reader, index);
  if (line == nullptr)
    return nullptr;

  // Read waypoint list
  // e.g. "Club day 4 Racing task","085PRI","083BOJ","170D_K","065SKY","0844YY", "0844YY"
  //       TASK NAME              , TAKEOFF, START  , TP1    , TP2    , FINISH ,  LANDING
  std::array<std::string_view, CUP_MAX_TPS> wps;
  CupSplitColumns(line, wps);

  std::size_t n_waypoints = 0;
  for (std::size_t i = 0; i < wps.size(); ++i)
    if (!wps[i].empty())
      n_waypoints = i + 1;

  // At least taskname and takeoff, start, finish and landing points are needed
  if (n_waypoints < 5)
    return nullptr;

  // Remove taskname, start point and landing point from count
  n_waypoints -= 3;

  SeeYouTaskInformation task_info;
  SeeYouTurnpointInformation turnpoint_infos[CUP_MAX_TPS];
  WaypointPtr waypoints_in_task[CUP_MAX_TPS];

  ParseCUTaskDetails(reader, task_info, turnpoint_infos);

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
    auto file_wp = file_waypoints.LookupName(string_converter.Convert(wps[i + 2]));
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
  FileReader file_reader{path};
  BufferedReader reader{file_reader};
  StringConverter string_converter;

  bool in_task_section = false;
  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (in_task_section) {
      // If the line starts with a string or "nothing" followed
      // by a comma it is a new task definition line
      if (line[0] == '\"' || line[0] == ',') {
        // If the task doesn't have a name inside the file
        if (line[0] == ',')
          result.emplace_back();
        else {
          std::string_view rest{line};
          const std::string_view task_name = CupNextColumn(rest);

          // Append task name to the list
          result.emplace_back(string_converter.Convert(task_name));
        }
      }
    } else if (StringIsEqualIgnoreCase(line, "-----Related Tasks-----")) {
      // Found the marker -> all following lines are task lines
      in_task_section = true;
    }
  }

  return result;
}
