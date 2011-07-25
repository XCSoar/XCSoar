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

#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/Navigation/Geometry/GeoVector.hpp"
#include "Math/Angle.hpp"
#include "Math/fixed.hpp"

#include "TestUtil.hpp"


#include <cstdio>

int main(int argc, char **argv)
{
  plan_tests(57);

  // test constructor
  GeoPoint p1(Angle::degrees(fixed(345.32)), Angle::degrees(fixed(-6.332)));
  ok1(equals(p1.Longitude, 345.32));
  ok1(equals(p1.Latitude, -6.332));

  // test normalize()
  p1.normalize();
  ok1(equals(p1.Longitude, -14.68));
  ok1(equals(p1.Latitude, -6.332));

  // test parametric()
  GeoPoint p2(Angle::degrees(fixed_two), Angle::degrees(fixed_one));
  GeoPoint p3 = p1.parametric(p2, fixed(5));
  ok1(equals(p3.Longitude, -4.68));
  ok1(equals(p3.Latitude, -1.332));

  // test interpolate
  GeoPoint p4 = p1.interpolate(p3, fixed_half);
  ok1(equals(p4.Longitude, -9.68));
  ok1(equals(p4.Latitude, -3.832));

  GeoPoint p5 = p1.interpolate(p3, fixed(0.25));
  ok1(equals(p5.Longitude, -12.18));
  ok1(equals(p5.Latitude, -5.082));

  // test *
  GeoPoint p6 = p2 * fixed(3.5);
  ok1(equals(p6.Longitude, 7));
  ok1(equals(p6.Latitude, 3.5));

  // test +
  p6 = p6 + p2;
  ok1(equals(p6.Longitude, 9));
  ok1(equals(p6.Latitude, 4.5));

  // test +=
  p6 += p2;
  ok1(equals(p6.Longitude, 11));
  ok1(equals(p6.Latitude, 5.5));

  // test -
  p6 = p6 - p2;
  ok1(equals(p6.Longitude, 9));
  ok1(equals(p6.Latitude, 4.5));

  // test sort()
  ok1(!p1.sort(p3));
  ok1(p3.sort(p1));
  ok1(!p1.sort(p4));
  ok1(p4.sort(p1));
  ok1(!p1.sort(p5));
  ok1(p5.sort(p1));
  ok1(!p4.sort(p3));
  ok1(p3.sort(p4));
  ok1(!p5.sort(p3));
  ok1(p3.sort(p5));
  ok1(!p5.sort(p4));
  ok1(p4.sort(p5));

  // test distance()
  //
  // note: distance between p1 and p4 and between p3 and p4 is not
  // the same due to linear interpolation instead of real geographic
  // intermediate point calculation
  ok1(equals(p2.distance(p6), 869326.653160));
  ok1(equals(p6.distance(p2), 869326.653160));
  ok1(equals(p1.distance(p5), 309562.219016));
  ok1(equals(p1.distance(p4), 619603.149273));
  ok1(equals(p1.distance(p3), 1240649.267606));
  ok1(equals(p3.distance(p4), 621053.760625));

  // test bearing()
  //
  // note: the bearings p1 -> p5, p5 -> p4 and so on are not the same due to
  // linear interpolation instead of real geographic intermediate point
  // calculation
  ok1(equals(p2.bearing(p6), 63.272424));
  ok1(equals(p6.bearing(p2), 243.608847));
  ok1(equals(p1.bearing(p5), 63.449343));
  ok1(equals(p1.bearing(p4), 63.582620));
  ok1(equals(p1.bearing(p3), 63.784526));
  ok1(equals(p5.bearing(p4), 63.466726));
  ok1(equals(p5.bearing(p3), 63.646072));
  ok1(equals(p4.bearing(p3), 63.540756));
  ok1(equals(p5.bearing(p6), 65.982854));
  ok1(equals(p2.bearing(p3), 250.786774));

  // test distance_bearing()
  // note: should be the same output as bearing() and distance()
  GeoVector v = p2.distance_bearing(p6);
  ok1(equals(v.Distance, 869326.653160));
  ok1(equals(v.Bearing, 63.272424));

  // test intermediate_point()
  GeoPoint p7(Angle::degrees(fixed_zero), Angle::degrees(fixed_zero));
  GeoPoint p8 = p7.intermediate_point(p2, fixed(100000));
  ok1(equals(p8.Longitude, 0.804342));
  ok1(equals(p8.Latitude, 0.402274));
  ok1(equals(p8.distance(p7), 100000));
  GeoPoint p9 = p7.intermediate_point(p2, fixed(100000000));
  ok1(equals(p9.Longitude, p2.Longitude));
  ok1(equals(p9.Latitude, p2.Latitude));

  // test projected_distance()
  ok1(equals(p8.projected_distance(p7, p2), 100000));
  ok1(equals(p4.projected_distance(p1, p3), 619599.304393));
  ok1(equals((p2 * fixed_two).projected_distance(p2, p6), 248567.832772));

  // Tests moved here from test_fixed.cpp
  GeoPoint l1(Angle::zero(), Angle::zero());
  GeoPoint l2(Angle::degrees(fixed(-0.3)), Angle::degrees(fixed(1.0)));
  GeoPoint l3(Angle::degrees(fixed(0.00001)), Angle::degrees(fixed_zero));
  GeoPoint l4(Angle::degrees(fixed(10)), Angle::degrees(fixed_zero));

  v = l1.distance_bearing(l2);
  printf("Dist %g bearing %d\n",
         FIXED_DOUBLE(v.Distance), FIXED_INT(v.Bearing.value_degrees()));
  // 116090 @ 343

  v = l1.distance_bearing(l3);
  printf("Dist %g bearing %d\n",
         FIXED_DOUBLE(v.Distance), FIXED_INT(v.Bearing.value_degrees()));
  ok(positive(v.Distance) && v.Distance < fixed_two, "earth distance short", 0);

  v = l1.distance_bearing(l4);
  printf("Dist %g bearing %d\n",
         FIXED_DOUBLE(v.Distance), FIXED_INT(v.Bearing.value_degrees()));

  return exit_status();
}
