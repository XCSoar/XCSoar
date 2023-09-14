// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCTrackTaskFile.hpp"
#include "XCTrackTaskDecoder.hpp"
#include "IGC/IGCDeclaration.hpp"
#include "io/FileReader.hxx"
#include "json/Parse.hxx"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/IntermediatePoint.hpp"
#include "Waypoint/Waypoint.hpp"

#include <boost/json/parser.hpp>

#include <cassert>

static auto
ParseJsonFile(Path path)
{
  FileReader r{path};
  return Json::Parse(r);
}

std::unique_ptr<OrderedTask>
XCTrackTaskFile::GetTask(const TaskBehaviour &task_behaviour,
                         [[maybe_unused]] const Waypoints *waypoints,
                         [[maybe_unused]] unsigned index) const
{
  assert(index == 0);

  return DecodeXCTrackTask(ParseJsonFile(path), task_behaviour);
}
