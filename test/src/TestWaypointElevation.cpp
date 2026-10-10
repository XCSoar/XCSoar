// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/**
 * @file TestWaypointElevation.cpp
 * @brief Unit tests for Waypoint elevation handling (GitHub issue #3286).
 *
 * These tests verify the expected behavior when accessing Waypoint::elevation:
 * - Waypoint::HasElevation() must be checked before using elevation
 * - Glide calculations should return NO_SOLUTION for unknown elevation
 * - Sea level (elevation=0 from the file) differs from unknown
 *
 * The glide helper functions here mirror the pattern used in production code
 * (InfoBoxes/Content/Places.cpp and InfoBoxes/Content/Alternate.cpp) to
 * document the expected behavior. The production code has UI dependencies
 * that prevent direct unit testing, but these tests verify the underlying
 * Waypoint class and the glide calculation pattern that all callers must
 * follow.
 */

#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/GlideSolvers/GlideSettings.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"

#include "TestUtil.hpp"

static GlideSettings glide_settings;
static GlidePolar glide_polar(1.0);

/**
 * Test that Waypoint::GetElevationOrZero() returns correct values
 * based on HasElevation().
 */
static void
TestGetElevationOrZero()
{
  Waypoint wp_with_elevation{GeoPoint(Angle::Degrees(7.0), Angle::Degrees(47.0))};
  wp_with_elevation.elevation = 500.0;
  wp_with_elevation.elevation_source = Waypoint::ElevationSource::FILE;

  Waypoint wp_without_elevation{GeoPoint(Angle::Degrees(7.1), Angle::Degrees(47.1))};
  wp_without_elevation.elevation = 999.0;  // Should be ignored
  wp_without_elevation.elevation_source = Waypoint::ElevationSource::NONE;

  ok1(equals(wp_with_elevation.GetElevationOrZero(), 500.0));
  ok1(equals(wp_without_elevation.GetElevationOrZero(), 0.0));
}

/**
 * Helper that mirrors the HasElevation() check pattern used in production
 * code (ComputeActiveWaypointGlide in Places.cpp). Returns invalid result
 * when elevation is unknown - this is the behavior all glide callers must
 * implement.
 */
static GlideResult
ComputeGlideToWaypoint(const GeoPoint &aircraft_location,
                       double aircraft_altitude,
                       const Waypoint &waypoint,
                       double safety_height)
{
  if (!waypoint.HasElevation()) {
    GlideResult result;
    result.Reset();
    return result;
  }

  const GlideState glide_state(
    aircraft_location.DistanceBearing(waypoint.location),
    waypoint.elevation + safety_height,
    aircraft_altitude,
    SpeedVector::Zero());

  return MacCready::Solve(glide_settings, glide_polar, glide_state);
}

/**
 * Test that glide calculation returns invalid result for waypoint
 * without elevation.
 */
static void
TestGlideWithoutElevation()
{
  const GeoPoint aircraft_location(Angle::Degrees(7.0), Angle::Degrees(47.0));
  const double aircraft_altitude = 1500.0;
  const double safety_height = 100.0;

  // Waypoint without elevation should return invalid result
  Waypoint wp_no_elev{GeoPoint(Angle::Degrees(7.1), Angle::Degrees(47.1))};
  wp_no_elev.elevation_source = Waypoint::ElevationSource::NONE;

  GlideResult result_no_elev = ComputeGlideToWaypoint(
    aircraft_location, aircraft_altitude, wp_no_elev, safety_height);

  ok1(!result_no_elev.IsDefined());
  ok1(result_no_elev.validity == GlideResult::Validity::NO_SOLUTION);
}

/**
 * Test that glide calculation returns valid result for waypoint
 * with elevation.
 */
static void
TestGlideWithElevation()
{
  const GeoPoint aircraft_location(Angle::Degrees(7.0), Angle::Degrees(47.0));
  const double aircraft_altitude = 1500.0;
  const double safety_height = 100.0;

  // Waypoint with elevation should return valid result
  Waypoint wp_with_elev{GeoPoint(Angle::Degrees(7.05), Angle::Degrees(47.05))};
  wp_with_elev.elevation = 500.0;
  wp_with_elev.elevation_source = Waypoint::ElevationSource::FILE;

  GlideResult result_with_elev = ComputeGlideToWaypoint(
    aircraft_location, aircraft_altitude, wp_with_elev, safety_height);

  ok1(result_with_elev.IsDefined());
  ok1(result_with_elev.validity == GlideResult::Validity::OK);
  // With 1500m altitude, 500m elevation + 100m safety = 600m target
  // Altitude difference should be positive (above glide)
  ok1(result_with_elev.altitude_difference > 0);
}

/**
 * Helper that mirrors the HasElevation() check pattern used in production
 * code (SolveManualAlternate in Alternate.cpp). The previous bug used
 * (HasElevation() ? elevation : 0) which incorrectly treated unknown
 * elevation as sea level.
 */
static GlideResult
SolveAlternateGlide(const GeoPoint &aircraft_location,
                    double aircraft_altitude,
                    const Waypoint &waypoint,
                    double safety_height)
{
  GlideResult solution;
  solution.Reset();

  if (!waypoint.HasElevation())
    return solution;

  const GlideState glide_state(
    aircraft_location.DistanceBearing(waypoint.location),
    waypoint.elevation + safety_height,
    aircraft_altitude,
    SpeedVector::Zero());

  return MacCready::Solve(glide_settings, glide_polar, glide_state);
}

/**
 * Test that alternate glide calculation behaves correctly for
 * waypoints with and without elevation.
 */
static void
TestAlternateGlide()
{
  const GeoPoint aircraft_location(Angle::Degrees(7.0), Angle::Degrees(47.0));
  const double aircraft_altitude = 1500.0;
  const double safety_height = 100.0;

  // Waypoint at 1000m elevation - should be reachable from 1500m
  Waypoint wp_high{GeoPoint(Angle::Degrees(7.02), Angle::Degrees(47.02))};
  wp_high.elevation = 1000.0;
  wp_high.elevation_source = Waypoint::ElevationSource::FILE;

  GlideResult result_high = SolveAlternateGlide(
    aircraft_location, aircraft_altitude, wp_high, safety_height);

  ok1(result_high.IsDefined());

  // Waypoint without elevation - should return invalid, not assume sea level
  Waypoint wp_unknown{GeoPoint(Angle::Degrees(7.02), Angle::Degrees(47.02))};
  wp_unknown.elevation_source = Waypoint::ElevationSource::NONE;

  GlideResult result_unknown = SolveAlternateGlide(
    aircraft_location, aircraft_altitude, wp_unknown, safety_height);

  ok1(!result_unknown.IsDefined());
}

/**
 * Test edge case: Waypoint at sea level (elevation = 0 from its file)
 * should be treated differently from unknown elevation.
 */
static void
TestSeaLevelVsUnknown()
{
  const GeoPoint aircraft_location(Angle::Degrees(7.0), Angle::Degrees(47.0));
  const double aircraft_altitude = 1000.0;
  const double safety_height = 50.0;

  // Coastal waypoint at actual sea level
  Waypoint wp_sea_level{GeoPoint(Angle::Degrees(7.1), Angle::Degrees(47.1))};
  wp_sea_level.elevation = 0.0;
  wp_sea_level.elevation_source = Waypoint::ElevationSource::FILE;

  // Waypoint with unknown elevation
  Waypoint wp_unknown{GeoPoint(Angle::Degrees(7.1), Angle::Degrees(47.1))};
  wp_unknown.elevation_source = Waypoint::ElevationSource::NONE;

  GlideResult result_sea_level = SolveAlternateGlide(
    aircraft_location, aircraft_altitude, wp_sea_level, safety_height);

  GlideResult result_unknown = SolveAlternateGlide(
    aircraft_location, aircraft_altitude, wp_unknown, safety_height);

  // Sea level waypoint should be reachable and have a valid solution
  ok1(result_sea_level.IsDefined());
  ok1(result_sea_level.IsOk());

  // Unknown elevation should return invalid solution
  ok1(!result_unknown.IsDefined());

  // Verify the sea level calculation shows positive altitude margin
  // From 1000m aircraft altitude to 50m target (0+safety), we can reach
  ok1(result_sea_level.altitude_difference > 0);
}

/**
 * Test Waypoint initialization defaults.
 */
static void
TestWaypointDefaults()
{
  Waypoint wp{GeoPoint(Angle::Degrees(7.0), Angle::Degrees(47.0))};

  // By default, a waypoint has no elevation
  ok1(!wp.HasElevation());

  // GetElevationOrZero should return 0 when HasElevation() is false
  ok1(equals(wp.GetElevationOrZero(), 0.0));
}

int main()
{
  plan_tests(15);

  glide_settings.SetDefaults();
  glide_polar.SetMC(1.0);

  TestWaypointDefaults();
  TestGetElevationOrZero();
  TestGlideWithoutElevation();
  TestGlideWithElevation();
  TestAlternateGlide();
  TestSeaLevelVsUnknown();

  return exit_status();
}
