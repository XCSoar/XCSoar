/* Copyright_License {

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

#include "Engine/Task/Points/TaskWaypoint.hpp"
#include "Geo/GeoVector.hpp"
#include "TestUtil.hpp"

class DummyTaskWaypoint: public TaskWaypoint
{
public:
  friend class TaskWaypointTest;

  DummyTaskWaypoint(TaskPointType _type, WaypointPtr &&wp)
    :TaskWaypoint(_type, std::move(wp)) {}

  GeoVector GetVectorRemaining(const GeoPoint &reference) const override {
    return GeoVector();
  }

  double GetElevation() const override {
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
  wp.name = _T("Test");
  wp.elevation = 42;

  DummyTaskWaypoint tw(TaskPointType::AST, WaypointPtr(new Waypoint(wp)));

  const Waypoint &wp2 = tw.GetWaypoint();
  ok1(wp2.name == _T("Test"));
  ok1(equals(tw.GetBaseElevation(), 42));
  ok1(equals(tw.GetBaseElevation(), wp2.elevation));
  ok1(equals(wp2.location, gp));
  ok1(equals(tw.GetLocation(), gp));
}

int main(int argc, char **argv)
{
  plan_tests(5);

  TaskWaypointTest test;
  test.Run();

  return exit_status();
}
