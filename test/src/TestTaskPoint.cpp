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

#include "Engine/Task/Tasks/BaseTask/TaskPoint.hpp"
#include "Engine/Navigation/Geometry/GeoVector.hpp"
#include "TestUtil.hpp"

class DummyTaskPoint: public TaskPoint
{
  AIRCRAFT_STATE dummy_state;

public:
  friend class TaskPointTest;

  DummyTaskPoint(enum type _type, const GeoPoint &_location,
                 const fixed _elevation)
    :TaskPoint(_type, _location, _elevation) {}

  virtual const GeoVector get_vector_remaining(const AIRCRAFT_STATE &) const {
    return GeoVector();
  }

  virtual const GeoVector get_vector_planned() const {
    return GeoVector();
  }

  virtual const GeoVector get_vector_travelled(const AIRCRAFT_STATE &) const {
    return GeoVector();
  }

  virtual bool has_entered() const {
    return false;
  }

  virtual const AIRCRAFT_STATE& get_state_entered() const {
    return dummy_state;
  }

  virtual fixed get_elevation() const {
    return fixed_zero;
  }
};

class TaskPointTest
{
public:
  void Run();
};

void
TaskPointTest::Run()
{
  GeoPoint gp1(Angle::degrees(fixed(20)), Angle::degrees(fixed(50)));
  GeoPoint gp2(Angle::degrees(fixed(21)), Angle::degrees(fixed(50)));

  DummyTaskPoint tp1(TaskPoint::AST, gp1, fixed(1234));
  DummyTaskPoint tp2(TaskPoint::AAT, gp2, fixed(1337));
  DummyTaskPoint tp3(TaskPoint::START, gp1, fixed(1234));
  DummyTaskPoint tp4(TaskPoint::FINISH, gp2, fixed(1337));

  ok1(tp1.IsIntermediatePoint());
  ok1(tp1.GetType() == TaskPoint::AST);
  ok1(equals(tp1.GetBaseElevation(), 1234));
  ok1(!tp1.has_target());
  ok1(equals(tp1.distance(gp2), gp1.distance(gp2)));
  ok1(equals(tp1.get_location().Latitude, gp1.Latitude));
  ok1(equals(tp1.get_location().Longitude, gp1.Longitude));

  ok1(tp2.IsIntermediatePoint());
  ok1(tp2.GetType() == TaskPoint::AAT);
  ok1(tp2.has_target());

  ok1(!tp3.IsIntermediatePoint());
  ok1(tp3.GetType() == TaskPoint::START);
  ok1(!tp3.has_target());

  ok1(!tp4.IsIntermediatePoint());
  ok1(tp4.GetType() == TaskPoint::FINISH);
  ok1(!tp4.has_target());
}

int main(int argc, char **argv)
{
  plan_tests(16);

  TaskPointTest test;
  test.Run();

  return exit_status();
}
