// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Task/Points/TaskPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "TestUtil.hpp"

class DummyTaskPoint: public TaskPoint
{
public:
  friend class TaskPointTest;

  DummyTaskPoint(TaskPointType _type, const GeoPoint &_location)
    :TaskPoint(_type, _location) {}

  GeoVector GetVectorRemaining([[maybe_unused]] const GeoPoint &reference) const noexcept override {
    return GeoVector();
  }

  double GetElevation() const noexcept override {
    return 0;
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
  GeoPoint gp1(Angle::Degrees(20), Angle::Degrees(50));
  GeoPoint gp2(Angle::Degrees(21), Angle::Degrees(50));

  DummyTaskPoint tp1(TaskPointType::AST, gp1);
  DummyTaskPoint tp2(TaskPointType::AAT, gp2);
  DummyTaskPoint tp3(TaskPointType::START, gp1);
  DummyTaskPoint tp4(TaskPointType::FINISH, gp2);

  ok1(tp1.IsIntermediatePoint());
  ok1(tp1.GetType() == TaskPointType::AST);
  ok1(!tp1.HasTarget());
  ok1(equals(tp1.Distance(gp2), gp1.Distance(gp2)));
  ok1(equals(tp1.GetLocation(), gp1));

  ok1(tp2.IsIntermediatePoint());
  ok1(tp2.GetType() == TaskPointType::AAT);
  ok1(tp2.HasTarget());

  ok1(!tp3.IsIntermediatePoint());
  ok1(tp3.GetType() == TaskPointType::START);
  ok1(!tp3.HasTarget());

  ok1(!tp4.IsIntermediatePoint());
  ok1(tp4.GetType() == TaskPointType::FINISH);
  ok1(!tp4.HasTarget());
}

int main()
{
  plan_tests(14);

  TaskPointTest test;
  test.Run();

  return exit_status();
}
