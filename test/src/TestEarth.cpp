// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo/Math.hpp"
#include "Geo/SimplifiedMath.hpp"
#include "TestUtil.hpp"

static void
TestLinearDistance()
{
  const GeoPoint lon_start(Angle::Degrees(90),
                           Angle::Zero());
  for (unsigned i = 0; i < 180; i += 5) {
    const GeoPoint lon_end(lon_start.longitude + Angle::Degrees(i),
                           lon_start.latitude);
    double distance = Distance(lon_start, lon_end);

    double min = 111300 * i;
    double max = 111340 * i;
    ok1(between(distance, min, max));

    distance = lon_start.DistanceS(lon_end);
    min = 111100 * i;
    max = 111200 * i;

    ok1(between(distance, min, max));
  }

  /* Unfortunately this test doesn't make sense
   * on the earth ellipsoid...
   */
  const GeoPoint lat_start(Angle::Zero(),
                           Angle::Zero());
  for (unsigned i = 0; i < 90; i += 5) {
    const GeoPoint lat_end(lat_start.longitude,
                           lat_start.latitude + Angle::Degrees(i));
    double distance = lat_start.DistanceS(lat_end);

    double min = 111100 * i;
    double max = 111200 * i;

    ok1(between(distance, min, max));
  }

}

int main()
{
  plan_tests(10 + 2 * 36 + 18);

  const GeoPoint a(Angle::Degrees(7.7061111111111114),
                   Angle::Degrees(51.051944444444445));
  const GeoPoint b(Angle::Degrees(7.599444444444444),
                   Angle::Degrees(51.099444444444444));
  const GeoPoint c(Angle::Degrees(4.599444444444444),
                   Angle::Degrees(47.099444444444444));

  double distance = Distance(a, b);
  ok1(distance > 9150 && distance < 9160);

  distance = a.DistanceS(b);
  ok1(distance > 9130 && distance < 9140);

  Angle bearing = Bearing(a, b);
  ok1(bearing.Degrees() > 304);
  ok1(bearing.Degrees() < 306);

  bearing = Bearing(b, a);
  ok1(bearing.Degrees() > 124);
  ok1(bearing.Degrees() < 126);

  distance = ProjectedDistance(a, b, a);
  ok1(is_zero(distance));
  distance = ProjectedDistance(a, b, b);

  ok1(distance > 9150 && distance < 9180);

  const GeoPoint middle(a.longitude.Fraction(b.longitude, 0.5),
                        a.latitude.Fraction(b.latitude, 0.5));
  distance = ProjectedDistance(a, b, middle);
  ok1(distance > 9150/2 && distance < 9180/2);

  double big_distance = Distance(a, c);
  ok1(big_distance > 494000 && big_distance < 495000);

  TestLinearDistance();

  return exit_status();
}
