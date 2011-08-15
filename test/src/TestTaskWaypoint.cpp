/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Engine/Task/Tasks/BaseTask/TaskWaypoint.hpp"
#include "Engine/Navigation/Geometry/GeoVector.hpp"
#include "TestUtil.hpp"

class DummyTaskWaypoint: public TaskWaypoint
{
  AircraftState dummy_state;

public:
  friend class TaskWaypointTest;

  DummyTaskWaypoint(enum type _type, const Waypoint & wp)
    :TaskWaypoint(_type, wp) {}

  virtual const GeoVector get_vector_remaining(const AircraftState &) const {
    return GeoVector();
  }

  virtual const GeoVector get_vector_planned() const {
    return GeoVector();
  }

  virtual const GeoVector get_vector_travelled(const AircraftState &) const {
    return GeoVector();
  }

  virtual bool has_entered() const {
    return false;
  }

  virtual const AircraftState& get_state_entered() const {
    return dummy_state;
  }

  virtual fixed get_elevation() const {
    return fixed_zero;
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
  GeoPoint gp(Angle::degrees(fixed(20)), Angle::degrees(fixed(50)));
  Waypoint wp(gp);
  wp.name = _T("Test");
  wp.altitude = fixed(42);

  DummyTaskWaypoint tw(TaskPoint::AST, wp);

  const Waypoint &wp2 = tw.get_waypoint();
  ok1(wp2.name == _T("Test"));
  ok1(equals(tw.GetBaseElevation(), 42));
  ok1(equals(tw.GetBaseElevation(), wp2.altitude));
  ok1(equals(wp2.location.Latitude, gp.Latitude));
  ok1(equals(wp2.location.Longitude, gp.Longitude));
  ok1(equals(tw.get_location().Latitude, gp.Latitude));
  ok1(equals(tw.get_location().Longitude, gp.Longitude));
}

int main(int argc, char **argv)
{
  plan_tests(7);

  TaskWaypointTest test;
  test.Run();

  return exit_status();
}
