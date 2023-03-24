// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo/UTM.hpp"
#include "Geo/GeoPoint.hpp"
#include "TestUtil.hpp"

#include <cstdio>

int main()
{
  plan_tests(5);

  GeoPoint p(Angle::Degrees(7), Angle::Degrees(50));
  UTM u = UTM::FromGeoPoint(p);

  ok1(u.zone_number == 32);
  ok1(u.zone_letter == 'U');
  ok1(equals(u.easting, 356670.9));
  ok1(equals(u.northing, 5540547.9));

  printf("%d%c E: %7f N: %7f\n", u.zone_number, u.zone_letter,
         (double)u.easting, (double)u.northing);

  p = u.ToGeoPoint();

  ok1(equals(p, 50, 7));

  printf("lon: %f - lat: %f\n", (double)p.longitude.Degrees(),
         (double)p.latitude.Degrees());

  return exit_status();
}
