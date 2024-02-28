// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "system/Path.hpp"
#include "Task/SaveFile.hpp"
#include "Task/LoadFile.hpp"
#include "system/FileUtil.hpp"

#include "TestUtil.hpp"

static TaskBehaviour task_behaviour;
static OrderedTaskSettings ordered_task_settings;
static constexpr Path task_path{_T("output/results/Test-Task.tsk")};

static constexpr GeoPoint
MakeGeoPoint(double longitude, double latitude) noexcept
{
  return {Angle::Degrees(longitude), Angle::Degrees(latitude)};
}

static Waypoint
MakeWaypoint(Waypoint wp, double altitude, tstring name, unsigned id) noexcept
{
  wp.name = name;
  wp.id = id;
  wp.elevation = altitude;
  wp.has_elevation = true;
  return wp;
}

static Waypoint
MakeWaypoint(double longitude, double latitude, double altitude, tstring name, unsigned id) noexcept
{
  return MakeWaypoint(Waypoint(MakeGeoPoint(longitude, latitude)), altitude, name, id);
}

template<typename... Args>
static WaypointPtr
MakeWaypointPtr(Args&&... args) noexcept
{
  return WaypointPtr(new Waypoint(MakeWaypoint(std::forward<Args>(args)...)));
}

static const auto wp1_str = _T("wp-01");
static const auto wp2_str = _T("wp-02");
static const auto wp3_str = _T("wp-03");

static const auto wp1 = MakeWaypointPtr(0, 45, 50, wp1_str, 100);
static const auto wp2 = MakeWaypointPtr(0, 45.3, 50,  wp2_str, 102);
static const auto wp3 = MakeWaypointPtr(0, 46, 50, wp3_str, 103);

static void
TestTaskSave()
{
  OrderedTask task_saved(task_behaviour);
  task_saved.Append(StartPoint(std::make_unique<CylinderZone>(wp1->location, 500),
                         WaypointPtr(wp1),
                         task_behaviour,
                         ordered_task_settings.start_constraints));
  task_saved.Append(AATPoint(std::make_unique<CylinderZone>(wp2->location, 10000),
                       WaypointPtr(wp2),
                       task_behaviour));
  task_saved.Append(FinishPoint(std::make_unique<CylinderZone>(wp3->location, 500),
                          WaypointPtr(wp3),
                          task_behaviour,
                          ordered_task_settings.finish_constraints));
  task_saved.SetActiveTaskPoint(1);
  task_saved.UpdateGeometry();
  SaveTask(task_path, task_saved);

  std::unique_ptr<OrderedTask> task_loaded = LoadTask(task_path, task_behaviour);
  ok1(task_loaded);

  ok1(task_loaded.get()->IsValidIndex(0));
  ok1(task_loaded.get()->IsValidIndex(1));
  ok1(task_loaded.get()->IsValidIndex(2));

  ok1(StringIsEqual(task_loaded.get()->GetTaskPoint(0).GetWaypoint().name.c_str(), wp1_str));
  ok1(StringIsEqual(task_loaded.get()->GetTaskPoint(1).GetWaypoint().name.c_str(), wp2_str));
  ok1(StringIsEqual(task_loaded.get()->GetTaskPoint(2).GetWaypoint().name.c_str(), wp3_str));

  ok1(task_loaded.get()->GetTaskPoint(0).GetWaypoint() == *wp1);
  ok1(task_loaded.get()->GetTaskPoint(1).GetWaypoint() == *wp2);
  ok1(task_loaded.get()->GetTaskPoint(2).GetWaypoint() == *wp3);
}

static void
TestAll()
{
  TestTaskSave();
}

int main()
{
  Directory::Create(Path{_T("output/results")});

  plan_tests(10);
  task_behaviour.SetDefaults();
  ordered_task_settings.SetDefaults();
  TestAll();
  return exit_status();
}
