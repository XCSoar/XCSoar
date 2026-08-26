// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "Weather/xctherm/XCThermGeoJSONCleanup.hpp"
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

static Ring
MakeBowtie()
{
  /* Classic self-crossing quad (figure-8). */
  Ring ring;
  ring.push_back(P(0, 0));
  ring.push_back(P(1, 1));
  ring.push_back(P(0, 1));
  ring.push_back(P(1, 0));
  ring.push_back(P(0, 0));
  return ring;
}

int main()
{
  plan_tests(14 + 4 + 8 + 3);

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

  /* --- Cleanup: drop holes --- */
  {
    WindBand band = MakeSquareBand(1.0, 2.0, 0, 0, 4, 4);
    Ring hole;
    hole.push_back(P(1.5, 1.5));
    hole.push_back(P(2.5, 1.5));
    hole.push_back(P(2.5, 2.5));
    hole.push_back(P(1.5, 2.5));
    hole.push_back(P(1.5, 1.5));
    band.polygons[0].push_back(std::move(hole));
    CleanBandPolygons(band);
    ok1(band.polygons.size() == 1);
    ok1(band.polygons[0].size() == 1);
  }

  /* --- Cleanup: split bowtie into two simple parts --- */
  {
    auto parts = CleanExterior(MakeBowtie());
    ok1(parts.size() == 2);
    ok1(parts[0].size() == 1 && parts[0][0].size() >= 4);
    ok1(parts[1].size() == 1 && parts[1][0].size() >= 4);
  }

  /* --- Cleanup: concave L becomes multiple convex pieces --- */
  {
    Ring ell;
    ell.push_back(P(0, 0));
    ell.push_back(P(2, 0));
    ell.push_back(P(2, 1));
    ell.push_back(P(1, 1));
    ell.push_back(P(1, 2));
    ell.push_back(P(0, 2));
    ell.push_back(P(0, 0));
    auto parts = CleanExterior(ell);
    ok1(parts.size() >= 2);
    ok1(parts.size() <= 4);
    /* Each piece is a single exterior ring with at least a triangle. */
    bool all_ok = !parts.empty();
    for (const auto &poly : parts)
      if (poly.size() != 1 || poly[0].size() < 4)
        all_ok = false;
    ok1(all_ok);
  }

  /* --- Cleanup: drop zero-area junk --- */
  {
    Ring flat;
    flat.push_back(P(0, 0));
    flat.push_back(P(1, 0));
    flat.push_back(P(2, 0));
    flat.push_back(P(0, 0));
    ok1(CleanExterior(flat).empty());
  }

  /* --- Sort bands by ascending |mid| --- */
  {
    ForecastLayer sorted;
    sorted.bands.push_back(MakeSquareBand(3.0, 4.0, 0, 0, 1, 1));   /* |mid|=3.5 */
    sorted.bands.push_back(MakeSquareBand(-1.0, -0.5, 0, 0, 1, 1)); /* |mid|=0.75 */
    sorted.bands.push_back(MakeSquareBand(0.2, 0.5, 0, 0, 1, 1));   /* |mid|=0.35 */
    SortBandsByAbsMid(sorted);
    ok1(equals((sorted.bands[0].min_ms + sorted.bands[0].max_ms) / 2, 0.35));
    ok1(equals((sorted.bands[2].min_ms + sorted.bands[2].max_ms) / 2, 3.5));
  }

  return exit_status();
}
