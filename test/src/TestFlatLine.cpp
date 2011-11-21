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

#include "Engine/Navigation/Flat/FlatLine.hpp"
#include "Engine/Navigation/Flat/FlatPoint.hpp"
#include "Math/Angle.hpp"
#include "TestUtil.hpp"

int main(int argc, char **argv)
{
  plan_tests(39);

  FlatPoint p1(fixed_one, fixed_one);
  FlatPoint p2(fixed_one, fixed_two);
  FlatPoint p3(fixed(3), fixed_ten);

  FlatLine l1(p1, p2); // 0, 1
  FlatLine l2(p1, p3); // 2, 9
  FlatLine l3(p2, p3); // 2, 8

  // test ave()
  FlatPoint av;
  av = l1.ave();
  ok1(equals(av.x, 1));
  ok1(equals(av.y, 1.5));
  av = l2.ave();
  ok1(equals(av.x, 2));
  ok1(equals(av.y, 5.5));
  av = l3.ave();
  ok1(equals(av.x, 2));
  ok1(equals(av.y, 6));

  // test angle()
  ok1(equals(l1.angle(), 90));
  ok1(equals(l2.angle(), 77.47119229084848923132012643871));
  ok1(equals(l3.angle(), 75.963756532073521417107679840837));

  // test dsq()
  ok1(equals(l1.dsq(), 1));
  ok1(equals(l2.dsq(), 85));
  ok1(equals(l3.dsq(), 68));

  // test d()
  ok1(equals(l1.d(), 1));
  ok1(equals(l2.d(), 9.2195444572928873100022742817628));
  ok1(equals(l3.d(), 8.2462112512353210996428197119482));

  // test dot()
  ok1(equals(l1.dot(l2), 9));
  ok1(equals(l1.dot(l3), 8));
  ok1(equals(l2.dot(l3), 76));

  // sub(), add(), rotate() and mul_y() not directly testable right
  // now because p1 and p2 are private

  // test intersect_czero()
  FlatPoint i1, i2;
  ok1(!l1.intersect_czero(fixed(0.9), i1, i2));

  ok1(l1.intersect_czero(fixed(1.8027756377319946465596106337352), i1, i2));
  ok1(equals(i1.x, 1));
  ok1(equals(i1.y, 1.5));
  ok1(equals(i2.x, 1));
  ok1(equals(i2.y, -1.5));

  ok1(l2.intersect_czero(fixed(5.8523499553598125545510491371143), i1, i2));
  ok1(equals(i1.x, 2));
  ok1(equals(i1.y, 5.5));
  ok1(equals(i2.x, -0.517647));
  ok1(equals(i2.y, -5.829411));

  // test intersect_circle()
  FlatPoint c(fixed_one, fixed(1.5));
  ok1(l1.intersect_circle(fixed(0.25), c, i1, i2));
  ok1(equals(i1.x, 1));
  ok1(equals(i1.y, 1.75));
  ok1(equals(i2.x, 1));
  ok1(equals(i2.y, 1.25));

  ok1(l1.intersect_circle(fixed_one, c, i1, i2));
  ok1(equals(i1.x, 1));
  ok1(equals(i1.y, 2.5));
  ok1(equals(i2.x, 1));
  ok1(equals(i2.y, 0.5));

  return exit_status();
}
