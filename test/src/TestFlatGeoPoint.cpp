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

#include "Geo/Flat/FlatGeoPoint.hpp"
#include "TestUtil.hpp"

int main(int argc, char **argv)
{
  plan_tests(37);

  FlatGeoPoint p1(1, 1);
  FlatGeoPoint p2(1, 2);
  FlatGeoPoint p3(3, 10);

  // test cross()
  ok1(p1.CrossProduct(p2) == 1);
  ok1(p2.CrossProduct(p1) == -1);
  ok1(p1.CrossProduct(p3) == 7);
  ok1(p3.CrossProduct(p1) == -7);
  ok1(p2.CrossProduct(p3) == 4);
  ok1(p3.CrossProduct(p2) == -4);

  // test dot()
  ok1(p1.DotProduct(p2) == 3);
  ok1(p2.DotProduct(p1) == 3);
  ok1(p1.DotProduct(p3) == 13);
  ok1(p3.DotProduct(p1) == 13);
  ok1(p2.DotProduct(p3) == 23);
  ok1(p3.DotProduct(p2) == 23);

  // test ==
  ok1(p1 == FlatGeoPoint(1, 1));
  ok1(p2 == FlatGeoPoint(1, 2));
  ok1(p3 == FlatGeoPoint(3, 10));

  // test <
  ok1(p2.Sort(p1));
  ok1(p3.Sort(p1));
  ok1(p3.Sort(p2));

  // test * (and ==)
  ok1(p1 * 2 == FlatGeoPoint(2, 2));
  ok1(p2 * 2 == FlatGeoPoint(2, 4));
  ok1(p3 * 2 == FlatGeoPoint(6, 20));

  // test +
  p2 = p2 + p1;
  ok1(p2.x == 2);
  ok1(p2.y == 3);

  // test -
  p2 = p2 - p1;
  ok1(p2.x == 1);
  ok1(p2.y == 2);

  // test distance_sq_to()
  ok1(p1.DistanceSquared(p2) == 1);
  ok1(p2.DistanceSquared(p1) == 1);
  ok1(p1.DistanceSquared(p3) == 85);
  ok1(p3.DistanceSquared(p1) == 85);
  ok1(p2.DistanceSquared(p3) == 68);
  ok1(p3.DistanceSquared(p2) == 68);

  // test distance_to()
  ok1(p1.Distance(p2) == 1);
  ok1(p2.Distance(p1) == 1);
  ok1(p1.Distance(p3) == 9);
  ok1(p3.Distance(p1) == 9);
  ok1(p2.Distance(p3) == 8);
  ok1(p3.Distance(p2) == 8);

  return exit_status();
}
