// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Task/TaskFileIGC.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCDeclaration.hpp"
#include "io/FileLineReader.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/IntermediatePoint.hpp"
#include "Waypoint/Waypoint.hpp"

#include <list>
#include <cassert>

static bool
ReadIGCDeclaration(Path path, IGCDeclarationHeader &header,
                   std::list<IGCDeclarationTurnpoint> &turnpoints)
try {
  // Create FileReader for reading the task
  FileLineReaderA reader(path);

  // Read IGC file
  char *line;
  bool header_found = false;
  while ((line = reader.ReadLine()) != nullptr) {
    // Skip lines which are not declaration records
    if (*line != 'C')
      continue;

    if (!header_found) {
      if (!IGCParseDeclarationHeader(line, header))
        return false;

      header_found = true;
      continue;
    }

    IGCDeclarationTurnpoint tp;
    if (IGCParseDeclarationTurnpoint(line, tp))
      turnpoints.emplace_back(tp);
  }

  return header_found;
} catch (...) {
  return false;
}

static WaypointPtr
MakeWaypoint(GeoPoint location, const char *name)
{
  Waypoint *wp = new Waypoint(location);
  wp->name = name;

  return WaypointPtr(wp);
}

std::unique_ptr<OrderedTask>
TaskFileIGC::GetTask(const TaskBehaviour &task_behaviour,
                     [[maybe_unused]] const Waypoints *waypoints, [[maybe_unused]] unsigned index) const
{
  assert(index == 0);

  IGCDeclarationHeader header;
  std::list<IGCDeclarationTurnpoint> turnpoints;

  if (!ReadIGCDeclaration(path, header, turnpoints))
    return nullptr;

  // Number of turnpoints including start and finish
  unsigned num_turnpoints = header.num_turnpoints + 2;

  if (num_turnpoints + 2 == turnpoints.size()) {
    // Remove takeoff and landing points from the turnpoints list
    turnpoints.pop_front();
    turnpoints.pop_back();
  } else if (num_turnpoints != turnpoints.size())
    // Declared number of turnpoints is not matching parsed number of turnpoints
    return nullptr;

  // Create a blank task
  auto task = std::make_unique<OrderedTask>(task_behaviour);
  AbstractTaskFactory &fact = task->GetFactory();

  unsigned i = 0;
  for (const auto &it : turnpoints) {
    StaticString<256> waypoint_name;
    if (!it.name.empty()) {
      waypoint_name.clear();
      waypoint_name.UnsafeAppendASCII(it.name);
    } else if (i == 0)
      waypoint_name = "Start";
    else if (i == num_turnpoints - 1)
      waypoint_name = "Finish";
    else
      waypoint_name.Format("%s #%u", "Turnpoint", i);

    auto wp = MakeWaypoint(it.location, waypoint_name.c_str());

    std::unique_ptr<OrderedTaskPoint> tp;

    if (i == 0)
      tp = fact.CreateStart(std::move(wp));
    else if (i == num_turnpoints - 1)
      tp = fact.CreateFinish(std::move(wp));
    else
      tp = fact.CreateIntermediate(std::move(wp));

    if (tp != nullptr) {
      fact.Append(*tp);
    }

    ++i;
  }

  return task;
}

std::vector<std::string>
TaskFileIGC::GetList() const
{
  // Open the IGC file
  FileLineReaderA reader(path);

  IGCDeclarationHeader header;

  // Search for declaration
  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (*line != 'C')
      continue;

    if (!IGCParseDeclarationHeader(line, header) ||
        header.num_turnpoints == 0)
      return {};

    std::vector<std::string> result;

    if (!header.task_name.empty() &&
        !StringIsEqual(header.task_name, "Task")) {
      // Remember the task name
      StaticString<256> task_name;
      task_name.clear();
      task_name.UnsafeAppendASCII(header.task_name.c_str());
      result.emplace_back(task_name.c_str());
    }

    return result;
  }

  return {};
}
