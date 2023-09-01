// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "harness_task.hpp"
#include "test_debug.hpp"
#include "harness_waypoints.hpp"

#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Task/Ordered/Points/StartPoint.hpp"
#include "Task/Ordered/Points/FinishPoint.hpp"
#include "Task/Ordered/Points/IntermediatePoint.hpp"
#include "util/StaticArray.hxx"

extern "C" {
#include "tap.h"
}

static TaskPointFactoryType
GetRandomType(const LegalPointSet &l)
{
  StaticArray<TaskPointFactoryType, LegalPointSet::N> types;
  l.CopyTo(std::back_inserter(types));
  return types[(rand() % types.size())];
}

bool test_task_bad(TaskManager& task_manager,
                   const Waypoints& waypoints)
{
  test_task_random(task_manager,waypoints,2);

  task_manager.SetFactory(TaskFactoryType::RACING);
  AbstractTaskFactory& fact = task_manager.GetFactory();

  const auto wp = random_waypoint(waypoints);

  ok (!fact.CreateFinish((TaskPointFactoryType)20, WaypointPtr(wp)),"bad finish type",0);
  ok (!fact.CreateStart((TaskPointFactoryType)20, WaypointPtr(wp)),"bad start type",0);
  ok (!fact.CreateIntermediate((TaskPointFactoryType)20, WaypointPtr(wp)),"bad intermediate type",0);

  // now create a taskpoint from FAI

  const TaskPointFactoryType s = GetRandomType(fact.GetIntermediateTypes());

  // test it is bad for AAT

  task_manager.SetFactory(TaskFactoryType::AAT);

  AbstractTaskFactory& bfact = task_manager.GetFactory();

  ok (!bfact.CreateIntermediate(s, WaypointPtr(wp)),"bad intermediate type (after task change)",0);

  bfact.Remove(1);
  bfact.UpdateStatsGeometry();
  ok (!IsError(bfact.Validate()), "ok with one tp", 0);

  bfact.Remove(1);
  bfact.UpdateStatsGeometry();
  ok (!IsError(bfact.Validate()), "ok with zero tps (just start and finish)", 0);

  ok(bfact.Remove(task_manager.GetOrderedTask().TaskSize() - 1, false),
     "remove finish manually", 0);
  bfact.UpdateStatsGeometry();
  ok (IsError(bfact.Validate()), "aat is invalid (no finish)", 0);

  return true;
}
