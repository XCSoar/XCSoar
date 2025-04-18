// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo/Flat/FlatPoint.hpp"
#include "Math/Angle.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(41);

  FlatPoint p1(1, 1);
  FlatPoint p2(1, 2);
  FlatPoint p3(3, 10);

  // test cross()
  ok1(equals(p1.CrossProduct(p2), 1));
  ok1(equals(p2.CrossProduct(p1), -1));
  ok1(equals(p1.CrossProduct(p3), 7));
  ok1(equals(p3.CrossProduct(p1), -7));
  ok1(equals(p2.CrossProduct(p3), 4));
  ok1(equals(p3.CrossProduct(p2), -4));

  // test mul_y()
  p2.MultiplyY(2);
  ok1(equals(p2.x, 1));
  ok1(equals(p2.y, 4));

  // test sub()
  p2 -= p1;
  ok1(equals(p2.x, 0));
  ok1(equals(p2.y, 3));

  // test add()
  p2 += p3;
  ok1(equals(p2.x, 3));
  ok1(equals(p2.y, 13));

  // test rotate()
  p2.Rotate(Angle::Degrees(-90));
  ok1(equals(p2.x, 13));
  ok1(equals(p2.y, -3));

  p2.Rotate(Angle::Degrees(45));
  p2.Rotate(Angle::Degrees(45));
  ok1(equals(p2.x, 3));
  ok1(equals(p2.y, 13));

  // test d()
  ok1(equals(p2.Distance(p3), 3));
  ok1(equals(p3.Distance(p2), 3));

  // test mag_sq()
  ok1(equals(p1.MagnitudeSquared(), 2));
  ok1(equals(p2.MagnitudeSquared(), 178));
  ok1(equals(p3.MagnitudeSquared(), 109));

  // test mag()
  ok1(equals(p1.Magnitude(), 1.4142135623730950488016887242097));
  ok1(equals(p2.Magnitude(), 13.341664064126333712489436272508));
  ok1(equals(p3.Magnitude(), 10.440306508910550179757754022548));

  // test dot()
  ok1(equals(p1.DotProduct(p2), 16));
  ok1(equals(p2.DotProduct(p1), 16));
  ok1(equals(p1.DotProduct(p3), 13));
  ok1(equals(p3.DotProduct(p1), 13));
  ok1(equals(p2.DotProduct(p3), 139));
  ok1(equals(p3.DotProduct(p2), 139));

  // test ==
  ok1(p1 == p1);
  ok1(p2 == p2);
  ok1(p3 == p3);
  /*
  // Test #2 fails due to floating point inaccuracies
  ok1(p1 == FlatPoint(1, 1));
  ok1(p2 == FlatPoint(3, 13));
  ok1(p3 == FlatPoint(3, 10));
  */

  // test *
  p2 = p3 * 1.5;
  ok1(equals(p2.x, 4.5));
  ok1(equals(p2.y, 15));

  // test +
  p2 = p1 + p3;
  ok1(equals(p2.x, 4));
  ok1(equals(p2.y, 11));

  // test +=
  p2 += p1;
  ok1(equals(p2.x, 5));
  ok1(equals(p2.y, 12));

  // test -
  p2 = p3 - p1;
  ok1(equals(p2.x, 2));
  ok1(equals(p2.y, 9));

  return exit_status();
}
