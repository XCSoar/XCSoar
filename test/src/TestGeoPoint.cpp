/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Geo/Math.hpp"
#include "Math/Angle.hpp"
#include "Math/fixed.hpp"

#include "TestUtil.hpp"


#include <cstdio>

int main(int argc, char **argv)
{
  plan_tests(66);

  // test constructor
  GeoPoint p1(Angle::Degrees(345.32), Angle::Degrees(-6.332));
  ok1(p1.IsValid());
  ok1(equals(p1, -6.332, 345.32));

  // test normalize()
  p1.Normalize();
  ok1(p1.IsValid());
  ok1(equals(p1, -6.332, -14.68));

  // test parametric()
  GeoPoint p2(Angle::Degrees(2), Angle::Degrees(1));
  GeoPoint p3 = p1.Parametric(p2, fixed(5));
  ok1(p2.IsValid());
  ok1(p3.IsValid());
  ok1(equals(p3, -1.332, -4.68));

  // test interpolate
  GeoPoint p4 = p1.Interpolate(p3, fixed(0.5));
  ok1(p4.IsValid());
  ok1(equals(p4, -3.832, -9.68));

  GeoPoint p5 = p1.Interpolate(p3, fixed(0.25));
  ok1(p5.IsValid());
  ok1(equals(p5, -5.082, -12.18));

  // test *
  GeoPoint p6 = p2 * fixed(3.5);
  ok1(p6.IsValid());
  ok1(equals(p6, 3.5, 7));

  // test +
  p6 = p6 + p2;
  ok1(p6.IsValid());
  ok1(equals(p6, 4.5, 9));

  // test +=
  p6 += p2;
  ok1(p6.IsValid());
  ok1(equals(p6, 5.5, 11));

  // test -
  p6 = p6 - p2;
  ok1(p6.IsValid());
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
  GeoPoint p7(Angle::Zero(), Angle::Zero());
  ok1(p7.IsValid());
  GeoPoint p8 = p7.IntermediatePoint(p2, fixed(100000));
  ok1(p8.IsValid());
  ok1(equals(p8, 0.402274, 0.804342));
  ok1(equals(p8.Distance(p7), 100000));
  GeoPoint p9 = p7.IntermediatePoint(p2, fixed(100000000));
  ok1(p9.IsValid());
  ok1(equals(p9, p2));

  // test projected_distance()
  ok1(equals(p8.ProjectedDistance(p7, p2), 100000));
  ok1(equals(p4.ProjectedDistance(p1, p3), 619599.304393));
  ok1(equals((p2 * fixed(2)).ProjectedDistance(p2, p6), 248567.832772));

  // Tests moved here from test_fixed.cpp
  GeoPoint l1(Angle::Zero(), Angle::Zero());
  ok1(l1.IsValid());
  GeoPoint l2(Angle::Degrees(-0.3), Angle::Degrees(1.0));
  ok1(l2.IsValid());
  GeoPoint l3(Angle::Degrees(0.00001), Angle::Zero());
  ok1(l3.IsValid());
  GeoPoint l4(Angle::Degrees(10), Angle::Zero());
  ok1(l4.IsValid());
  l4.SetInvalid();
  ok1(!l4.IsValid());

  bool find_lat_lon_okay = true;
  for (Angle bearing = Angle::Zero(); bearing < Angle::FullCircle();
      bearing += Angle::Degrees(5)) {
    GeoPoint p_test = FindLatitudeLongitude(p1, bearing, fixed(50000));
    find_lat_lon_okay = equals(p_test.Distance(p1), 50000) && find_lat_lon_okay;
  }
  ok1(find_lat_lon_okay);

  v = l1.DistanceBearing(l2);
  printf("Dist %g bearing %d\n",
         FIXED_DOUBLE(v.distance), FIXED_INT(v.bearing.Degrees()));
  // 116090 @ 343

  v = l1.DistanceBearing(l3);
  printf("Dist %g bearing %d\n",
         FIXED_DOUBLE(v.distance), FIXED_INT(v.bearing.Degrees()));
  ok(positive(v.distance) && v.distance < fixed(2), "earth distance short", 0);

  v = l1.DistanceBearing(l4);
  printf("Dist %g bearing %d\n",
         FIXED_DOUBLE(v.distance), FIXED_INT(v.bearing.Degrees()));

  GeoPoint p10(GeoPoint::Invalid());
  ok1(!p10.IsValid());


  return exit_status();
}
