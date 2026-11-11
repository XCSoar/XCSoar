// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Projection/Projection.hpp"
#include "Projection/MapLod.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/Quadrilateral.hpp"
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

static void
test_map_lod()
{
  ok1(MapLod(0) == 0);
  ok1(MapLod(1200) == 0);
  ok1(MapLod(1200.1) == 1);
  ok1(MapLod(2500) == 1);
  ok1(MapLod(2500.1) == 2);
  ok1(MapLod(5000) == 2);
  ok1(MapLod(5000.1) == 3);
  ok1(MapLod(15000) == 3);
  ok1(MapLod(15000.1) == 4);
  ok1(MapLodFillLevel(20000) == 3);
  ok1(MapLodFillLevel(600) == 0);
  ok1(MapLodRepresentativeMapScale(2) == 3750.);

  const GeoBounds pad(GeoPoint(Angle::Degrees(-2), Angle::Degrees(2)),
                      GeoPoint(Angle::Degrees(2), Angle::Degrees(-2)));
  const GeoQuadrilateral inside{
    GeoPoint(Angle::Degrees(-1), Angle::Degrees(1)),
    GeoPoint(Angle::Degrees(1), Angle::Degrees(1)),
    GeoPoint(Angle::Degrees(-1), Angle::Degrees(-1)),
    GeoPoint(Angle::Degrees(1), Angle::Degrees(-1)),
  };
  const GeoQuadrilateral outside{
    GeoPoint(Angle::Degrees(-3), Angle::Degrees(0)),
    GeoPoint(Angle::Degrees(0), Angle::Degrees(0)),
    GeoPoint(Angle::Degrees(-3), Angle::Degrees(-1)),
    GeoPoint(Angle::Degrees(0), Angle::Degrees(-1)),
  };
  ok1(MapLodQuadInside(pad, inside));
  ok1(!MapLodQuadInside(pad, outside));
  ok1(!MapLodQuadInside(GeoBounds::Invalid(), inside));
}

int main()
{
  plan_tests(4 + 15);

  test_simple();
  test_map_lod();

  return exit_status();
}
