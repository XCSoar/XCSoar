// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Projection/Projection.hpp"
#include "TestUtil.hpp"

static void
TestGeoScreenCouple(const Projection prj, const GeoPoint geo,
                    int x, int y)
{
  auto tmp_pt = prj.GeoToScreen(geo);
  ok1(tmp_pt.x == x);
  ok1(tmp_pt.y == y);

  GeoPoint tmp_geo = prj.ScreenToGeo({x, y});
  ok1(equals(tmp_geo.latitude, geo.latitude));
  ok1(equals(tmp_geo.longitude, geo.longitude));
}

static void
test_simple()
{
  Projection prj;
  prj.SetGeoLocation(GeoPoint::Zero());

  TestGeoScreenCouple(prj, GeoPoint(Angle::Zero(),
                                    Angle::Zero()), 0, 0);
}

int main()
{
  plan_tests(4);

  test_simple();

  return exit_status();
}
