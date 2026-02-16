// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Task/Points/TaskWaypoint.hpp"
#include "Geo/GeoVector.hpp"
#include "TestUtil.hpp"

class DummyTaskWaypoint: public TaskWaypoint
{
public:
  friend class TaskWaypointTest;

  DummyTaskWaypoint(TaskPointType _type, WaypointPtr &&wp)
    :TaskWaypoint(_type, std::move(wp)) {}

  GeoVector GetVectorRemaining([[maybe_unused]] const GeoPoint &reference) const noexcept override {
    return GeoVector();
  }

  double GetElevation() const noexcept override {
    return 0;
  }
};

class TaskWaypointTest
{
public:
  void Run();
};

void
TaskWaypointTest::Run()
{
  GeoPoint gp(Angle::Degrees(20), Angle::Degrees(50));
  Waypoint wp(gp);
  wp.name = "Test";
  wp.elevation = 42;
  wp.has_elevation = true;

  DummyTaskWaypoint tw(TaskPointType::AST, WaypointPtr(new Waypoint(wp)));

  const Waypoint &wp2 = tw.GetWaypoint();
  ok1(wp2.name == "Test");
  ok1(equals(tw.GetBaseElevation(), 42));
  ok1(wp2.has_elevation);
  ok1(equals(tw.GetBaseElevation(), wp2.elevation));
  ok1(equals(wp2.location, gp));
  ok1(equals(tw.GetLocation(), gp));
}

int main()
{
  plan_tests(6);

  TaskWaypointTest test;
  test.Run();

  return exit_status();
}
