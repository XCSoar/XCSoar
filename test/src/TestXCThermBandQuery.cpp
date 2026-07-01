// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "TestUtil.hpp"

using namespace XCThermGeoJSON;

/* Build a square exterior-ring band [min,max] m/s covering the lon/lat
   box [west,east] x [south,north]. */
static WindBand
MakeSquareBand(double min_ms, double max_ms,
               double west, double south, double east, double north)
{
  Ring ring;
  ring.push_back(GeoPoint(Angle::Degrees(west), Angle::Degrees(south)));
  ring.push_back(GeoPoint(Angle::Degrees(east), Angle::Degrees(south)));
  ring.push_back(GeoPoint(Angle::Degrees(east), Angle::Degrees(north)));
  ring.push_back(GeoPoint(Angle::Degrees(west), Angle::Degrees(north)));
  ring.push_back(GeoPoint(Angle::Degrees(west), Angle::Degrees(south)));

  WindBand band;
  band.min_ms = min_ms;
  band.max_ms = max_ms;
  band.polygons.push_back({std::move(ring)});  // one polygon, one ring
  return band;
}

static GeoPoint
P(double lon, double lat)
{
  return GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
}

int main()
{
  plan_tests(14 + 4);

  ForecastLayer layer;
  /* Two disjoint bands side by side:
       A: +0.5..+1.0 over lon[0,1] lat[0,1]
       B: +2.0..+3.0 over lon[2,3] lat[0,1]                       */
  layer.bands.push_back(MakeSquareBand(0.5, 1.0, 0, 0, 1, 1));
  layer.bands.push_back(MakeSquareBand(2.0, 3.0, 2, 0, 3, 1));

  double lo = -99, hi = -99;

  /* --- Point inside band A --- */
  /* This is the exact contract the [[gnu::pure]] bug broke: the
     out-params MUST be written when the function returns true. */
  ok1(FindBandAtPoint(layer, P(0.5, 0.5), lo, hi));
  ok1(equals(lo, 0.5));
  ok1(equals(hi, 1.0));

  /* --- Point inside band B --- */
  lo = hi = -99;
  ok1(FindBandAtPoint(layer, P(2.5, 0.5), lo, hi));
  ok1(equals(lo, 2.0));
  ok1(equals(hi, 3.0));

  /* --- Point in the gap between A and B: no match, out-params kept --- */
  lo = hi = -99;
  ok1(!FindBandAtPoint(layer, P(1.5, 0.5), lo, hi));
  ok1(equals(lo, -99));
  ok1(equals(hi, -99));

  /* --- Point well outside everything --- */
  ok1(!FindBandAtPoint(layer, P(50, 50), lo, hi));

  /* --- Empty layer never matches --- */
  ForecastLayer empty;
  ok1(!FindBandAtPoint(empty, P(0.5, 0.5), lo, hi));

  /* --- Nested bands: the most extreme (largest |midpoint|) wins --- */
  ForecastLayer nested;
  /* Big weak band over lon[0,4], strong band nested inside lon[1,2]. */
  nested.bands.push_back(MakeSquareBand(0.2, 0.5, 0, 0, 4, 4));
  nested.bands.push_back(MakeSquareBand(3.0, 4.0, 1, 1, 2, 2));
  lo = hi = -99;
  /* (1.5,1.5) is inside BOTH → expect the strong band. */
  ok1(FindBandAtPoint(nested, P(1.5, 1.5), lo, hi));
  ok1(equals(lo, 3.0));
  ok1(equals(hi, 4.0));

  /* --- Polygon with a hole: interior ring excludes the center --- */
  ForecastLayer holed;
  {
    WindBand band = MakeSquareBand(1.0, 2.0, 0, 0, 4, 4);
    Ring hole;
    hole.push_back(GeoPoint(Angle::Degrees(1.5), Angle::Degrees(1.5)));
    hole.push_back(GeoPoint(Angle::Degrees(2.5), Angle::Degrees(1.5)));
    hole.push_back(GeoPoint(Angle::Degrees(2.5), Angle::Degrees(2.5)));
    hole.push_back(GeoPoint(Angle::Degrees(1.5), Angle::Degrees(2.5)));
    hole.push_back(GeoPoint(Angle::Degrees(1.5), Angle::Degrees(1.5)));
    band.polygons[0].push_back(std::move(hole));
    holed.bands.push_back(std::move(band));
  }
  lo = hi = -99;
  ok1(!FindBandAtPoint(holed, P(2.0, 2.0), lo, hi));
  ok1(FindBandAtPoint(holed, P(0.5, 0.5), lo, hi));
  ok1(equals(lo, 1.0));
  ok1(equals(hi, 2.0));

  return exit_status();
}
