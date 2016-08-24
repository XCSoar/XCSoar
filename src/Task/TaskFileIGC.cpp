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

#include "Task/TaskFileIGC.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCDeclaration.hpp"
#include "IO/FileLineReader.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/IntermediatePoint.hpp"
#include "Waypoint/Waypoint.hpp"

#include <list>
#include <assert.h>

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
    if (*line != _T('C'))
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
} catch (const std::runtime_error &) {
  return false;
}

static WaypointPtr
MakeWaypoint(GeoPoint location, const TCHAR *name)
{
  Waypoint *wp = new Waypoint(location);
  wp->name = name;

  /* we don't know the elevation, so we just set it to zero; this is
     not correct, but better than leaving it uninitialised */
  wp->elevation = 0;

  return WaypointPtr(wp);
}

OrderedTask*
TaskFileIGC::GetTask(const TaskBehaviour &task_behaviour,
                     const Waypoints *waypoints, unsigned index) const
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
  OrderedTask *task = new OrderedTask(task_behaviour);
  AbstractTaskFactory &fact = task->GetFactory();

  unsigned i = 0;
  for (const auto &it : turnpoints) {
    StaticString<256> waypoint_name;
    if (!it.name.empty()) {
      waypoint_name.clear();
      waypoint_name.UnsafeAppendASCII(it.name);
    } else if (i == 0)
      waypoint_name = _T("Start");
    else if (i == num_turnpoints - 1)
      waypoint_name = _T("Finish");
    else
      waypoint_name.Format(_T("%s #%u"), _T("Turnpoint"), i);

    auto wp = MakeWaypoint(it.location, waypoint_name.c_str());

    OrderedTaskPoint *tp;

    if (i == 0)
      tp = fact.CreateStart(std::move(wp));
    else if (i == num_turnpoints - 1)
      tp = fact.CreateFinish(std::move(wp));
    else
      tp = fact.CreateIntermediate(std::move(wp));

    if (tp != nullptr) {
      fact.Append(*tp);
      delete tp;
    }

    ++i;
  }

  return task;
}

unsigned
TaskFileIGC::Count()
try {
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
      return 0;

    if (!header.task_name.empty() &&
        !StringIsEqual(header.task_name, "Task")) {
      // Remember the task name
      StaticString<256> task_name;
      task_name.clear();
      task_name.UnsafeAppendASCII(header.task_name.c_str());
      namesuffixes.append(_tcsdup(task_name.c_str()));
    }

    return 1;
  }

  return 0;
} catch (const std::runtime_error &e) {
  return 0;
}
