// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Route/ReachFan.hpp"
#include "Engine/Route/ReachResult.hpp"
#include "Engine/Route/TerrainRoute.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/SpeedVector.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "GlideSolvers/GlideSettings.hpp"
#include "Terrain/RasterMap.hpp"
#include "TestUtil.hpp"

extern "C" {
#include "jasper/jas_seq.h"
}

#include <cstdlib>
#include <memory>
#include <optional>

static constexpr unsigned N = 64;

static GeoPoint
At(const RasterMap &map, unsigned x, unsigned y) noexcept
{
  return map.GetProjection().UnprojectCoarse(SignedRasterLocation(x, y));
}

static int
HeightAt(const RasterMap &map, unsigned x, unsigned y) noexcept
{
  return map.GetHeight(At(map, x, y)).GetValueOr0();
}

/**
 * One 64x64 tile covering half a degree. The fine tile is loaded, so
 * intersection tests do not fall back to the coarse overview.
 */
static void
LoadTile(RasterMap &map, jas_matrix_t &heights)
{
  auto &cache = map.GetTileCache();
  cache.SetSize({N, N}, {N, N}, {1, 1});
  cache.SetLatLonBounds(-0.25, 0.25, -0.25, 0.25);
  cache.PutOverviewTile(0, {0, 0}, {N, N}, heights);
  cache.PollTiles(SignedRasterLocation(N / 2, N / 2), 1000);
  cache.PutTileData(0, heights);
  map.UpdateProjection();
}

static std::unique_ptr<jas_matrix_t, void (*)(jas_matrix_t *)>
MakeHeights(int fill)
{
  jas_matrix_t *m = jas_matrix_create(N, N);
  for (unsigned y = 0; y < N; ++y)
    for (unsigned x = 0; x < N; ++x)
      jas_matrix_set(m, y, x, fill);
  return {m, jas_matrix_destroy};
}

/** North-south wall with open ground on either side. */
static void
PaintRidge(jas_matrix_t &m, int ridge_height)
{
  for (unsigned y = 20; y <= 44; ++y)
    for (unsigned x = 30; x <= 33; ++x)
      jas_matrix_set(&m, y, x, ridge_height);
}

static RoutePlannerConfig
ReachConfig(RoutePlannerConfig::ReachMode mode) noexcept
{
  RoutePlannerConfig config;
  config.SetDefaults();
  config.mode = RoutePlannerConfig::Mode::TERRAIN;
  config.allow_climb = false;
  config.reach_calc_mode = mode;
  return config;
}

static ReachFan
Solve(TerrainRoute &route, const RoutePlannerConfig &config,
      const AGeoPoint &origin, int height_min_working, bool working)
{
  GlideSettings settings;
  settings.SetDefaults();
  const GlidePolar polar(0);
  route.UpdatePolar(settings, config, polar, polar,
                    SpeedVector(Angle::Zero(), 0), height_min_working);
  return route.SolveReach(origin, config, INT_MAX, true, working);
}

static ReachFan
SolveWind(TerrainRoute &route, const RoutePlannerConfig &config,
          const AGeoPoint &origin, const SpeedVector &wind)
{
  GlideSettings settings;
  settings.SetDefaults();
  const GlidePolar polar(0);
  route.UpdatePolar(settings, config, polar, polar, wind, 0);
  return route.SolveReach(origin, config, INT_MAX, true, false);
}

static std::optional<ReachResult>
Arrival(const TerrainRoute &route, const ReachFan &fan,
        const GeoPoint &dest, int dest_alt)
{
  return fan.FindPositiveArrival(AGeoPoint(dest, dest_alt),
                                 route.GetReachPolar());
}

static void
TestRidgeIntersection(const RasterMap &map)
{
  const GeoPoint origin = At(map, 12, 32);
  const GeoPoint behind = At(map, 50, 32);

  ok1(HeightAt(map, 12, 32) == 0);
  ok1(HeightAt(map, 31, 32) == 2000);

  /* Shallow slope from 800 m meets the 2000 m ridge. */
  const auto hit = map.FirstIntersection(origin, 800, behind, 0,
                                         800, 800, 150);
  ok1(static_cast<bool>(hit));

  /* The same line from 4000 m stays above the ridge. */
  const auto clear = map.FirstIntersection(origin, 4000, behind, 0,
                                           4000, 4000, 150);
  ok1(!static_cast<bool>(clear));
}

static void
TestFlatReach(const RasterMap &map)
{
  const auto config = ReachConfig(RoutePlannerConfig::ReachMode::STRAIGHT);
  TerrainRoute route;
  route.SetTerrain(&map);

  const AGeoPoint origin(At(map, 16, 32), 350);
  const auto fan = Solve(route, config, origin, 0, false);

  const auto near = Arrival(route, fan, At(map, 22, 32), 0);
  ok1(near && near->IsReachableDirect());
  ok1(near->IsReachableTerrain());
  ok1(std::abs(near->direct - near->terrain) < 50);

  const auto far = Arrival(route, fan, At(map, 63, 32), 0);
  ok1(far && !far->IsReachableDirect());
  ok1(far->terrain_valid == ReachResult::Validity::UNREACHABLE);
}

static void
TestWindReach(const RasterMap &map)
{
  const auto config = ReachConfig(RoutePlannerConfig::ReachMode::STRAIGHT);
  TerrainRoute route;
  route.SetTerrain(&map);

  const AGeoPoint origin(At(map, 32, 32), 800);
  /* Bearing is the direction the wind comes from: from the west. */
  const auto fan = SolveWind(route, config, origin,
                             SpeedVector(Angle::Degrees(270), 8));

  const auto downwind = Arrival(route, fan, At(map, 48, 32), 0);
  const auto upwind = Arrival(route, fan, At(map, 16, 32), 0);
  ok1(downwind && upwind);
  ok1(downwind->direct > upwind->direct);
}

static void
TestWorkingFloor(const RasterMap &map)
{
  const auto config = ReachConfig(RoutePlannerConfig::ReachMode::STRAIGHT);
  TerrainRoute route;
  route.SetTerrain(&map);

  const AGeoPoint origin(At(map, 8, 32), 1200);
  const auto terrain = Solve(route, config, origin, 0, false);
  const auto working = Solve(route, config, origin, 900, true);

  bool separated = false;
  for (unsigned x = 12; x < N - 1; x += 2) {
    const auto dest = At(map, x, 32);
    const auto in_terrain = Arrival(route, terrain, dest, 0);
    const auto in_working = Arrival(route, working, dest, 0);
    if (in_terrain && in_terrain->IsReachableTerrain() &&
        in_working && !in_working->IsReachableTerrain()) {
      separated = true;
      break;
    }
  }
  ok1(separated);
}

static void
TestTooLow(const RasterMap &map)
{
  const auto config = ReachConfig(RoutePlannerConfig::ReachMode::STRAIGHT);
  TerrainRoute route;
  route.SetTerrain(&map);

  GlideSettings settings;
  settings.SetDefaults();
  const GlidePolar polar(0);
  route.UpdatePolar(settings, config, polar, polar,
                    SpeedVector(Angle::Zero(), 0), 0);

  /* At safety height the fan is not scanned. */
  const AGeoPoint origin(At(map, 32, 32),
                         config.safety_height_terrain);
  ReachFan fan;
  ok1(!fan.Solve(origin, route.GetReachPolar(), &map, true));

  const auto arrival = fan.FindPositiveArrival(AGeoPoint(At(map, 40, 32), 0),
                                               route.GetReachPolar());
  ok1(arrival);
  ok1(arrival->terrain_valid == ReachResult::Validity::INVALID);
}

static void
TestRidgeReach(const RasterMap &map)
{
  TerrainRoute route;
  route.SetTerrain(&map);

  const AGeoPoint origin(At(map, 12, 32), 2000);
  const GeoPoint behind = At(map, 50, 32);

  const auto straight_config =
    ReachConfig(RoutePlannerConfig::ReachMode::STRAIGHT);
  const auto straight = Solve(route, straight_config, origin, 0, false);
  const auto straight_arrival = Arrival(route, straight, behind, 0);
  ok1(straight_arrival);
  ok1(!straight_arrival->IsReachableTerrain());

  const auto turning_config =
    ReachConfig(RoutePlannerConfig::ReachMode::TURNING);
  const auto turning = Solve(route, turning_config, origin, 0, false);
  const auto turning_arrival = Arrival(route, turning, behind, 0);
  ok1(turning_arrival);
  ok1(turning_arrival->IsReachableTerrain());
}

int
main()
{
  plan_tests(4 + 5 + 2 + 1 + 3 + 4);

  auto flat = MakeHeights(0);
  RasterMap flat_map;
  LoadTile(flat_map, *flat);
  TestFlatReach(flat_map);
  TestWindReach(flat_map);
  TestWorkingFloor(flat_map);
  TestTooLow(flat_map);

  auto ridge = MakeHeights(0);
  PaintRidge(*ridge, 2000);
  RasterMap ridge_map;
  LoadTile(ridge_map, *ridge);
  TestRidgeIntersection(ridge_map);
  TestRidgeReach(ridge_map);

  return exit_status();
}
