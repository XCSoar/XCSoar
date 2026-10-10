// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Task/Points/TaskWaypoint.hpp"
#include "Engine/Task/Unordered/GotoTask.hpp"
#include "Engine/Task/TaskBehaviour.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
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
  wp.elevation_source = Waypoint::ElevationSource::FILE;

  DummyTaskWaypoint tw(TaskPointType::AST, WaypointPtr(new Waypoint(wp)));

  const Waypoint &wp2 = tw.GetWaypoint();
  ok1(wp2.name == "Test");
  ok1(equals(tw.GetBaseElevation(), 42));
  ok1(wp2.HasElevation());
  ok1(equals(tw.GetBaseElevation(), wp2.elevation));
  ok1(equals(wp2.location, gp));
  ok1(equals(tw.GetLocation(), gp));
}

static WaypointPtr
MakeGotoWaypoint(double elevation, bool has_elevation) noexcept
{
  Waypoint wp(GeoPoint(Angle::Degrees(7.05), Angle::Degrees(47.05)));
  wp.elevation = elevation;
  wp.elevation_source = has_elevation
    ? Waypoint::ElevationSource::FILE
    : Waypoint::ElevationSource::NONE;
  return WaypointPtr(new Waypoint(wp));
}

static AircraftState
MakeAircraft() noexcept
{
  AircraftState aircraft;
  aircraft.Reset();
  aircraft.location = GeoPoint(Angle::Degrees(7.0), Angle::Degrees(47.0));
  aircraft.altitude = 2000;
  return aircraft;
}

/**
 * A goto with no stored elevation must not be solved as 0 m MSL.
 * A stored elevation of 0 m (sea level) still produces a glide.
 */
static void
TestGotoElevation()
{
  TaskBehaviour behaviour;
  behaviour.SetDefaults();

  Waypoints waypoints;
  GotoTask task(behaviour, waypoints);
  const AircraftState aircraft = MakeAircraft();
  GlidePolar polar(0);

  ok1(task.DoGoto(MakeGotoWaypoint(999, false)));

  GlideResult total, leg;
  task.GlideSolutionRemaining(aircraft, polar, total, leg);
  ok1(!total.IsDefined());
  ok1(!leg.IsDefined());
  ok1(equals(task.CalcGradient(aircraft), 0));

  ok1(task.DoGoto(MakeGotoWaypoint(500, true)));
  task.GlideSolutionRemaining(aircraft, polar, total, leg);
  ok1(total.IsOk());
  ok1(leg.IsOk());
  ok1(task.CalcGradient(aircraft) > 0);

  ok1(task.DoGoto(MakeGotoWaypoint(0, true)));
  task.GlideSolutionRemaining(aircraft, polar, total, leg);
  ok1(total.IsOk());
}

int main()
{
  plan_tests(16);

  TaskWaypointTest test;
  test.Run();
  TestGotoElevation();

  return exit_status();
}
