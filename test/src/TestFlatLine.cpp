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

#include "Geo/Flat/FlatLine.hpp"
#include "Geo/Flat/FlatPoint.hpp"
#include "Math/Angle.hpp"
#include "TestUtil.hpp"

int main(int argc, char **argv)
{
  plan_tests(39);

  FlatPoint p1(1, 1);
  FlatPoint p2(1, 2);
  FlatPoint p3(3, 10);

  FlatLine l1(p1, p2); // 0, 1
  FlatLine l2(p1, p3); // 2, 9
  FlatLine l3(p2, p3); // 2, 8

  // test GetMiddle()
  FlatPoint av;
  av = l1.GetMiddle();
  ok1(equals(av.x, 1));
  ok1(equals(av.y, 1.5));
  av = l2.GetMiddle();
  ok1(equals(av.x, 2));
  ok1(equals(av.y, 5.5));
  av = l3.GetMiddle();
  ok1(equals(av.x, 2));
  ok1(equals(av.y, 6));

  // test GetAngle()
  ok1(equals(l1.GetAngle(), 90));
  ok1(equals(l2.GetAngle(), 77.47119229084848923132012643871));
  ok1(equals(l3.GetAngle(), 75.963756532073521417107679840837));

  // test GetSquaredDistance()
  ok1(equals(l1.GetSquaredDistance(), 1));
  ok1(equals(l2.GetSquaredDistance(), 85));
  ok1(equals(l3.GetSquaredDistance(), 68));

  // test GetDistance()
  ok1(equals(l1.GetDistance(), 1));
  ok1(equals(l2.GetDistance(), 9.2195444572928873100022742817628));
  ok1(equals(l3.GetDistance(), 8.2462112512353210996428197119482));

  // test DotProduct()
  ok1(equals(l1.DotProduct(l2), 9));
  ok1(equals(l1.DotProduct(l3), 8));
  ok1(equals(l2.DotProduct(l3), 76));

  // sub(), add(), rotate() and mul_y() not directly testable right
  // now because p1 and p2 are private

  // test IntersectOriginCircle()
  FlatPoint i1, i2;
  ok1(!l1.IntersectOriginCircle(0.9, i1, i2));

  ok1(l1.IntersectOriginCircle(1.8027756377319946465596106337352, i1, i2));
  ok1(equals(i1.x, 1));
  ok1(equals(i1.y, 1.5));
  ok1(equals(i2.x, 1));
  ok1(equals(i2.y, -1.5));

  ok1(l2.IntersectOriginCircle(5.8523499553598125545510491371143, i1, i2));
  ok1(equals(i1.x, 2));
  ok1(equals(i1.y, 5.5));
  ok1(equals(i2.x, -0.517647));
  ok1(equals(i2.y, -5.829411));

  // test IntersectCircle()
  FlatPoint c(1, 1.5);
  ok1(l1.IntersectCircle(0.25, c, i1, i2));
  ok1(equals(i1.x, 1));
  ok1(equals(i1.y, 1.75));
  ok1(equals(i2.x, 1));
  ok1(equals(i2.y, 1.25));

  ok1(l1.IntersectCircle(1, c, i1, i2));
  ok1(equals(i1.x, 1));
  ok1(equals(i1.y, 2.5));
  ok1(equals(i2.x, 1));
  ok1(equals(i2.y, 0.5));

  return exit_status();
}
