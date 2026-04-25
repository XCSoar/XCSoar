// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Unit tests for Engine/Task/PathSolvers TaskDijkstraMin / TaskDijkstraMax.
// (IsolineCrossingFinder is exercised from AATIsolineSegment in flight code;
// synthetic OrderedTask fixtures often fail GeoEllipse flat constraints in
// debug builds.)

#include "Engine/Task/PathSolvers/TaskDijkstraMax.hpp"
#include "Engine/Task/PathSolvers/TaskDijkstraMin.hpp"
#include "Geo/SearchPointVector.hpp"

#include "TestUtil.hpp"

static constexpr GeoPoint
MakeGeoPoint(double longitude, double latitude) noexcept
{
  return {Angle::Degrees(longitude), Angle::Degrees(latitude)};
}

/**
 * Minimum-distance search should pick the stage-0 point closest to the aircraft
 * when both connect to the same stage-1 point.
 */
static void
TestDijkstraMinPicksNearerFirstStage()
{
  const GeoPoint L = MakeGeoPoint(8.0, 47.0);
  const GeoPoint A = MakeGeoPoint(8.05, 47.0);
  const GeoPoint B = MakeGeoPoint(8.50, 47.0);
  const GeoPoint C = MakeGeoPoint(8.06, 47.01);

  SearchPointVector boundary0;
  boundary0.emplace_back(A);
  boundary0.emplace_back(B);
  SearchPointVector boundary1;
  boundary1.emplace_back(C);

  TaskDijkstraMin d;
  d.SetTaskSize(2);
  d.SetBoundary(0, boundary0);
  d.SetBoundary(1, boundary1);

  const SearchPoint loc(L);
  ok1(d.DistanceMin(loc));
  ok1(equals(d.GetSolution(0).GetLocation().Distance(L),
             A.Distance(L)));
  ok1(equals(d.GetSolution(1).GetLocation().Distance(C), 0.));
}

/**
 * Maximum-distance search completes for two stages; solution picks boundary
 * points (stage-0 choice also depends on AddZeroStartEdges index bias).
 */
static void
TestDijkstraMaxTwoStages()
{
  const GeoPoint A = MakeGeoPoint(9.0, 48.0);
  const GeoPoint B = MakeGeoPoint(9.40, 48.0);
  const GeoPoint C = MakeGeoPoint(9.10, 48.02);

  SearchPointVector boundary0;
  boundary0.emplace_back(A);
  boundary0.emplace_back(B);
  SearchPointVector boundary1;
  boundary1.emplace_back(C);

  TaskDijkstraMax d;
  d.SetTaskSize(2);
  d.SetBoundary(0, boundary0);
  d.SetBoundary(1, boundary1);

  ok1(d.DistanceMax());
  const GeoPoint s0 = d.GetSolution(0).GetLocation();
  ok1(equals(s0.Distance(A), 0.) || equals(s0.Distance(B), 0.));
  ok1(equals(d.GetSolution(1).GetLocation().Distance(C), 0.));
}

/** No aircraft location: zero-start edges with index bias; still solvable. */
static void
TestDijkstraMinInvalidStartUsesZeroEdges()
{
  const GeoPoint P = MakeGeoPoint(10.0, 49.0);
  const GeoPoint Q = MakeGeoPoint(10.02, 49.01);

  SearchPointVector b0, b1;
  b0.emplace_back(P);
  b1.emplace_back(Q);

  TaskDijkstraMin d;
  d.SetTaskSize(2);
  d.SetBoundary(0, b0);
  d.SetBoundary(1, b1);

  ok1(d.DistanceMin(SearchPoint::Invalid()));
}

int
main()
{
  plan_tests(7);

  TestDijkstraMinPicksNearerFirstStage();
  TestDijkstraMaxTwoStages();
  TestDijkstraMinInvalidStartUsesZeroEdges();

  return exit_status();
}
