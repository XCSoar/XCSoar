/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Engine/Navigation/Flat/FlatPoint.hpp"
#include "Math/Angle.hpp"
#include "TestUtil.hpp"

int main(int argc, char **argv)
{
  plan_tests(41);

  FlatPoint p1(fixed_one, fixed_one);
  FlatPoint p2(fixed_one, fixed_two);
  FlatPoint p3(fixed(3), fixed_ten);

  // test cross()
  ok1(equals(p1.cross(p2), 1));
  ok1(equals(p2.cross(p1), -1));
  ok1(equals(p1.cross(p3), 7));
  ok1(equals(p3.cross(p1), -7));
  ok1(equals(p2.cross(p3), 4));
  ok1(equals(p3.cross(p2), -4));

  // test mul_y()
  p2.mul_y(fixed_two);
  ok1(equals(p2.x, 1));
  ok1(equals(p2.y, 4));

  // test sub()
  p2.sub(p1);
  ok1(equals(p2.x, 0));
  ok1(equals(p2.y, 3));

  // test add()
  p2.add(p3);
  ok1(equals(p2.x, 3));
  ok1(equals(p2.y, 13));

  // test rotate()
  p2.rotate(Angle::degrees(fixed(-90)));
  ok1(equals(p2.x, 13));
  ok1(equals(p2.y, -3));

  p2.rotate(Angle::degrees(fixed(45)));
  p2.rotate(Angle::degrees(fixed(45)));
  ok1(equals(p2.x, 3));
  ok1(equals(p2.y, 13));

  // test d()
  ok1(equals(p2.d(p3), 3));
  ok1(equals(p3.d(p2), 3));

  // test mag_sq()
  ok1(equals(p1.mag_sq(), 2));
  ok1(equals(p2.mag_sq(), 178));
  ok1(equals(p3.mag_sq(), 109));

  // test mag()
  ok1(equals(p1.mag(), 1.4142135623730950488016887242097));
  ok1(equals(p2.mag(), 13.341664064126333712489436272508));
  ok1(equals(p3.mag(), 10.440306508910550179757754022548));

  // test dot()
  ok1(equals(p1.dot(p2), 16));
  ok1(equals(p2.dot(p1), 16));
  ok1(equals(p1.dot(p3), 13));
  ok1(equals(p3.dot(p1), 13));
  ok1(equals(p2.dot(p3), 139));
  ok1(equals(p3.dot(p2), 139));

  // test ==
  ok1(p1 == p1);
  ok1(p2 == p2);
  ok1(p3 == p3);
  /*
  // Test #2 fails due to floating point inaccuracies
  ok1(p1 == FlatPoint(fixed_one, fixed_one));
  ok1(p2 == FlatPoint(fixed(3), fixed(13)));
  ok1(p3 == FlatPoint(fixed(3), fixed_ten));
  */

  // test *
  p2 = p3 * fixed(1.5);
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
