// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Unit tests for Engine/Airspace: AirspaceAltitude, AirspaceCircle,
// AirspacePolygon (horizontal containment and segment queries).

#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspaceIntersectionVector.hpp"
#include "Engine/Airspace/AirspacePolygon.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Geo/GeoVector.hpp"
#include "Navigation/Aircraft.hpp"

#include "TestUtil.hpp"

#include <vector>

static constexpr GeoPoint
GP(double lon_deg, double lat_deg) noexcept
{
  return {Angle::Degrees(lon_deg), Angle::Degrees(lat_deg)};
}

static void
TestAirspaceAltitudeMsl()
{
  AirspaceAltitude a{};
  a.reference = AltitudeReference::MSL;
  a.altitude = 3500.;

  AltitudeState st{};
  st.altitude = 3200.;
  st.altitude_agl = 0.;

  ok1(equals(a.GetAltitude(st), 3500.));
  ok1(a.IsAbove(st));
  ok1(!a.IsBelow(st));
}

static void
TestAirspaceAltitudeAglGetAltitude()
{
  AirspaceAltitude a{};
  a.reference = AltitudeReference::AGL;
  a.altitude_above_terrain = 800.;
  a.SetGroundLevel(2100.);

  AltitudeState st{};
  st.altitude = 4000.;
  st.altitude_agl = 1200.;

  /* 800 + (4000 - 1200) = 3600 MSL for the AGL surface at aircraft. */
  ok1(equals(a.GetAltitude(st), 3600.));
  ok1(a.NeedGroundLevel());
}

static void
TestAirspaceAltitudeTerrainIsBelow()
{
  AirspaceAltitude gnd{};
  gnd.reference = AltitudeReference::AGL;
  gnd.altitude_above_terrain = 0.;
  gnd.SetGroundLevel(500.);

  AltitudeState st{};
  st.altitude = 100.;
  st.altitude_agl = 50.;

  ok1(gnd.IsTerrain());
  /* Terrain ceiling branch: always "below" for vertical comparisons. */
  ok1(gnd.IsBelow(st));
}

static void
TestAirspaceCircleInsideAndClosest()
{
  const GeoPoint center = GP(8.4, 47.2);
  constexpr double r_m = 3000.;
  const AirspaceCircle as(center, r_m);

  ok1(as.Inside(center));

  const GeoPoint outside = GeoVector(r_m * 3, Angle::Degrees(0)).EndPoint(center);
  ok1(!as.Inside(outside));

  FlatProjection proj(center);
  ok1(as.ClosestPoint(center, proj) == center);

  const GeoPoint on_circle = as.ClosestPoint(outside, proj);
  /* IntermediatePoint on the ellipsoid; allow small geodesic tolerance. */
  ok1(between(on_circle.DistanceS(center), r_m - 2., r_m + 2.));
}

static void
TestAirspaceCircleIntersectsSegment()
{
  const GeoPoint center = GP(9.0, 48.0);
  constexpr double r_m = 4000.;
  const AirspaceCircle as(center, r_m);

  const GeoPoint w = GeoVector(25000., Angle::Degrees(270.)).EndPoint(center);
  const GeoPoint e = GeoVector(25000., Angle::Degrees(90.)).EndPoint(center);

  FlatProjection proj(center);
  const auto hits = as.Intersects(w, e, proj);
  ok1(!hits.empty());
}

static void
TestAirspacePolygonInsideAndClosest()
{
  /* Small lon/lat square; PolygonInterior uses planar lon/lat. */
  const GeoPoint p0 = GP(10.0, 49.0);
  const GeoPoint p1 = GP(10.04, 49.0);
  const GeoPoint p2 = GP(10.04, 49.04);
  const GeoPoint p3 = GP(10.0, 49.04);

  const std::vector<GeoPoint> corners = {p0, p1, p2, p3};
  AirspacePolygon poly(corners);

  const GeoPoint inside = GP(10.02, 49.02);
  ok1(poly.Inside(inside));

  ok1(!poly.Inside(GP(9.5, 49.02)));

  FlatProjection proj(inside);
  /* Return value must be used: GetBoundingBox projects the border; the call
     is [[gnu::pure]] and may be dropped if unused. */
  const FlatBoundingBox bb = poly.GetBoundingBox(proj);
  ok1(bb.lower_left.x <= bb.upper_right.x);
  const GeoPoint near_out = GP(10.06, 49.02);
  const GeoPoint cp = poly.ClosestPoint(near_out, proj);
  /* Nearest point on the east edge; flat-space NearestPoint + Unproject. */
  ok1(between(cp.longitude.Degrees(), 10.035, 10.045) &&
      between(cp.latitude.Degrees(), 49.015, 49.025));
}

int
main()
{
  plan_tests(16);

  TestAirspaceAltitudeMsl();
  TestAirspaceAltitudeAglGetAltitude();
  TestAirspaceAltitudeTerrainIsBelow();
  TestAirspaceCircleInsideAndClosest();
  TestAirspaceCircleIntersectsSegment();
  TestAirspacePolygonInsideAndClosest();

  return exit_status();
}
