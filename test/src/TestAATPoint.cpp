// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Settings.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Geo/Flat/TaskProjection.hpp"
#include "TestUtil.hpp"

static TaskBehaviour task_behaviour;
static OrderedTaskSettings ordered_task_settings;
static GlidePolar glide_polar(0);

static constexpr GeoPoint
MakeGeoPoint(double longitude, double latitude) noexcept
{
  return {Angle::Degrees(longitude), Angle::Degrees(latitude)};
}

static Waypoint
MakeWaypoint(Waypoint wp, double altitude) noexcept
{
  wp.elevation = altitude;
  wp.has_elevation = true;
  return wp;
}

static Waypoint
MakeWaypoint(double longitude, double latitude, double altitude) noexcept
{
  return MakeWaypoint(Waypoint(MakeGeoPoint(longitude, latitude)), altitude);
}

template<typename... Args>
static WaypointPtr
MakeWaypointPtr(Args&&... args) noexcept
{
  return WaypointPtr(new Waypoint(MakeWaypoint(std::forward<Args>(args)...)));
}

static const auto wp1 = MakeWaypointPtr(0, 45, 50);
static const auto wp2 = MakeWaypointPtr(0, 45.3, 50);
static const auto wp3 = MakeWaypointPtr(0, 46, 50);

static void
TestAATPoint()
{
  OrderedTask task(task_behaviour);
  task.Append(StartPoint(std::make_unique<CylinderZone>(wp1->location, 500),
                         WaypointPtr(wp1),
                         task_behaviour,
                         ordered_task_settings.start_constraints));
  task.Append(AATPoint(std::make_unique<CylinderZone>(wp2->location, 10000),
                       WaypointPtr(wp2),
                       task_behaviour));
  task.Append(FinishPoint(std::make_unique<CylinderZone>(wp3->location, 500),
                          WaypointPtr(wp3),
                          task_behaviour,
                          ordered_task_settings.finish_constraints));
  task.SetActiveTaskPoint(1);
  task.UpdateGeometry();
  ok1(!IsError(task.CheckTask()));

  AATPoint &ap = (AATPoint &)task.GetPoint(1);

  ok1(!ap.IsTargetLocked());
  ok1(equals(ap.GetTargetLocation(), wp2->location));
  ap.LockTarget(true);
  ok1(ap.IsTargetLocked());
  ok1(equals(ap.GetTargetLocation(), wp2->location));

  GeoPoint target = MakeGeoPoint(0, 45.31);
  ap.SetTarget(target);
  ok1(ap.IsTargetLocked());
  ok1(equals(ap.GetTargetLocation(), wp2->location));

  ap.SetTarget(target, true);
  ok1(ap.IsTargetLocked());
  ok1(equals(ap.GetTargetLocation(), target));

  RangeAndRadial rar = ap.GetTargetRangeRadial();
  ok1(equals(rar.range, 0.1112, 1000));
  ok1(equals(rar.radial.Degrees(), 0, 200));

  target = MakeGeoPoint(0, 45.29);
  ap.SetTarget(target, true);
  rar = ap.GetTargetRangeRadial();
  ok1(equals(rar.range, -0.1112, 1000));
  ok1(equals(rar.radial.Degrees(), 180, 200) ||
      equals(rar.radial.Degrees(), -180, 200));

  target = MakeGeoPoint(-0.05, 45.3);
  ap.SetTarget(target, true);
  rar = ap.GetTargetRangeRadial();
  ok1(equals(rar.range, 0.39217));
  ok1(equals(rar.radial.Degrees(), -89.98));

  target = MakeGeoPoint(0.05, 45.3);
  ap.SetTarget(target, true);
  rar = ap.GetTargetRangeRadial();
  ok1(equals(rar.range, 0.39217));
  ok1(equals(rar.radial.Degrees(), 89.98));

  for (int radial = -170; radial <= 170; radial += 10) {
    const Angle radial2 = Angle::Degrees(radial);

    for (int range = 10; range <= 100; range += 10) {
      const double range2((radial >= -90 && radial <= 90
                           ? range : -range) / 100.);

      ap.SetTarget(RangeAndRadial{range2, radial2}, task.GetTaskProjection());
      rar = ap.GetTargetRangeRadial();
      ok1(equals(rar.range, range2, 100));
      ok1(equals(rar.radial.Degrees(), radial2.Degrees(), 100));
    }
  }
}

static void
TestAll()
{
  TestAATPoint();
}

int main()
{
  plan_tests(717);

  task_behaviour.SetDefaults();
  ordered_task_settings.SetDefaults();

  TestAll();

  return exit_status();
}
