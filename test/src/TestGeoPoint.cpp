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
  plan_tests(46);

  // test constructor
  GeoPoint p1(Angle::degrees(fixed(345.32)), Angle::degrees(fixed(-6.332)));
  ok1(equals(p1, -6.332, 345.32));

  // test normalize()
  p1.Normalize();
  ok1(equals(p1, -6.332, -14.68));

  // test parametric()
  GeoPoint p2(Angle::degrees(fixed_two), Angle::degrees(fixed_one));
  GeoPoint p3 = p1.Parametric(p2, fixed(5));
  ok1(equals(p3, -1.332, -4.68));

  // test interpolate
  GeoPoint p4 = p1.Interpolate(p3, fixed_half);
  ok1(equals(p4, -3.832, -9.68));

  GeoPoint p5 = p1.Interpolate(p3, fixed(0.25));
  ok1(equals(p5, -5.082, -12.18));

  // test *
  GeoPoint p6 = p2 * fixed(3.5);
  ok1(equals(p6, 3.5, 7));

  // test +
  p6 = p6 + p2;
  ok1(equals(p6, 4.5, 9));

  // test +=
  p6 += p2;
  ok1(equals(p6, 5.5, 11));

  // test -
  p6 = p6 - p2;
  ok1(equals(p6, 4.5, 9));

  // test sort()
  ok1(!p1.Sort(p3));
  ok1(p3.Sort(p1));
  ok1(!p1.Sort(p4));
  ok1(p4.Sort(p1));
  ok1(!p1.Sort(p5));
  ok1(p5.Sort(p1));
  ok1(!p4.Sort(p3));
  ok1(p3.Sort(p4));
  ok1(!p5.Sort(p3));
  ok1(p3.Sort(p5));
  ok1(!p5.Sort(p4));
  ok1(p4.Sort(p5));

  // test distance()
  //
  // note: distance between p1 and p4 and between p3 and p4 is not
  // the same due to linear interpolation instead of real geographic
  // intermediate point calculation
  ok1(equals(p2.Distance(p6), 869326.653160));
  ok1(equals(p6.Distance(p2), 869326.653160));
  ok1(equals(p1.Distance(p5), 309562.219016));
  ok1(equals(p1.Distance(p4), 619603.149273));
  ok1(equals(p1.Distance(p3), 1240649.267606));
  ok1(equals(p3.Distance(p4), 621053.760625));

  // test bearing()
  //
  // note: the bearings p1 -> p5, p5 -> p4 and so on are not the same due to
  // linear interpolation instead of real geographic intermediate point
  // calculation
  ok1(equals(p2.Bearing(p6), 63.272424));
  ok1(equals(p6.Bearing(p2), 243.608847));
  ok1(equals(p1.Bearing(p5), 63.449343));
  ok1(equals(p1.Bearing(p4), 63.582620));
  ok1(equals(p1.Bearing(p3), 63.784526));
  ok1(equals(p5.Bearing(p4), 63.466726));
  ok1(equals(p5.Bearing(p3), 63.646072));
  ok1(equals(p4.Bearing(p3), 63.540756));
  ok1(equals(p5.Bearing(p6), 65.982854));
  ok1(equals(p2.Bearing(p3), 250.786774));

  // test distance_bearing()
  // note: should be the same output as bearing() and distance()
  GeoVector v = p2.DistanceBearing(p6);
  ok1(equals(v.distance, 869326.653160));
  ok1(equals(v.bearing, 63.272424));

  // test intermediate_point()
  GeoPoint p7(Angle::degrees(fixed_zero), Angle::degrees(fixed_zero));
  GeoPoint p8 = p7.IntermediatePoint(p2, fixed(100000));
  ok1(equals(p8, 0.402274, 0.804342));
  ok1(equals(p8.Distance(p7), 100000));
  GeoPoint p9 = p7.IntermediatePoint(p2, fixed(100000000));
  ok1(equals(p9, p2));

  // test projected_distance()
  ok1(equals(p8.ProjectedDistance(p7, p2), 100000));
  ok1(equals(p4.ProjectedDistance(p1, p3), 619599.304393));
  ok1(equals((p2 * fixed_two).ProjectedDistance(p2, p6), 248567.832772));

  // Tests moved here from test_fixed.cpp
  GeoPoint l1(Angle::zero(), Angle::zero());
  GeoPoint l2(Angle::degrees(fixed(-0.3)), Angle::degrees(fixed(1.0)));
  GeoPoint l3(Angle::degrees(fixed(0.00001)), Angle::degrees(fixed_zero));
  GeoPoint l4(Angle::degrees(fixed(10)), Angle::degrees(fixed_zero));

  v = l1.DistanceBearing(l2);
  printf("Dist %g bearing %d\n",
         FIXED_DOUBLE(v.distance), FIXED_INT(v.bearing.value_degrees()));
  // 116090 @ 343

  v = l1.DistanceBearing(l3);
  printf("Dist %g bearing %d\n",
         FIXED_DOUBLE(v.distance), FIXED_INT(v.bearing.value_degrees()));
  ok(positive(v.distance) && v.distance < fixed_two, "earth distance short", 0);

  v = l1.DistanceBearing(l4);
  printf("Dist %g bearing %d\n",
         FIXED_DOUBLE(v.distance), FIXED_INT(v.bearing.value_degrees()));

  return exit_status();
}
