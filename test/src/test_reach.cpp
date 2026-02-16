// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#include <iostream>
#include <fstream>
#include "Printing.hpp"
#define DO_PRINT
#include "TestUtil.hpp"
#include "Route/TerrainRoute.hpp"
#include "Route/ReachFan.hpp"
#include "Engine/Route/ReachResult.hpp"
#include "Terrain/RasterMap.hpp"
#include "Terrain/Loader.hpp"
#include "system/ConvertPathName.hpp"
#include "Compatibility/path.h"
#include "GlideSolvers/GlideSettings.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Geo/SpeedVector.hpp"
#include "Operation/Operation.hpp"
#include "system/FileUtil.hpp"
#include "util/PrintException.hxx"

#include <zzip/zzip.h>

#include <string.h>

static void
test_reach(const RasterMap &map, double mwind, double mc, double height_min_working)
{
  GlideSettings settings;
  settings.SetDefaults();
  RoutePlannerConfig config;
  config.SetDefaults();

  GlidePolar polar(mc);
  SpeedVector wind(Angle::Degrees(0), mwind);
  TerrainRoute route;
  route.UpdatePolar(settings, config, polar, polar, wind, height_min_working);
  route.SetTerrain(&map);

  GeoPoint origin(map.GetMapCenter());

  int horigin = map.GetHeight(origin).GetValueOr0() + 1000;
  AGeoPoint aorigin(origin, horigin);

  const auto reach_terrain = route.SolveReach(aorigin, config, INT_MAX,
                                              true, false);
  PrintHelper::print(reach_terrain);

  const auto reach_working = route.SolveReach(aorigin, config, INT_MAX,
                                              true, true);
  PrintHelper::print(reach_working);

  {
    Directory::Create(Path("output/results"));
    std::ofstream fout("output/results/terrain.txt");
    unsigned nx = 100;
    unsigned ny = 100;
    for (unsigned i=0; i< nx; ++i) {
      for (unsigned j=0; j< ny; ++j) {
        double fx = (double)i / (nx - 1) * 2 - 1;
        double fy = (double)j / (ny - 1) * 2 - 1;
        GeoPoint x(origin.longitude + Angle::Degrees(0.6 * fx),
                   origin.latitude + Angle::Degrees(0.6 * fy));
        int h = map.GetInterpolatedHeight(x).GetValueOr0();
        AGeoPoint adest(x, h);
        const auto reach = reach_terrain.FindPositiveArrival(adest,
                                                             route.GetReachPolar());
        if ((i % 5 == 0) && (j % 5 == 0)) {
          AGeoPoint ao2(x, h + 1000);
          [[maybe_unused]] auto reach2 =
                             route.SolveReach(ao2, config, INT_MAX,
                                              true, false);
        }
        fout << x.longitude.Degrees() << " "
             << x.latitude.Degrees() << " "
             << h << " " << (int)reach->terrain << "\n";
      }
      fout << "\n";
    }
    fout << "\n";
  }

  //  double pd = map.PixelDistance(origin, 1);
  //  printf("# pixel size %g\n", (double)pd);
}

int
main(int argc, char **argv)
try {
  static const char hc_path[] = "tmp/map.xcm";
  const char *map_path;
  if ((argc<2) || !strlen(argv[1])) {
    map_path = hc_path;
  } else {
    map_path = argv[1];
  }

  ZZIP_DIR *dir = zzip_dir_open(map_path, nullptr);
  if (dir == nullptr) {
    fprintf(stderr, "Failed to open %s\n", map_path);
    return EXIT_FAILURE;
  }

  RasterMap map;

  {
    NullOperationEnvironment operation;
    LoadTerrainOverview(dir, map.GetTileCache(), operation);
  }

  map.UpdateProjection();

  SharedMutex mutex;
  do {
    UpdateTerrainTiles(dir, map.GetTileCache(), mutex,
                       map.GetProjection(),
                       map.GetMapCenter(), 50000);
  } while (map.IsDirty());
  zzip_dir_close(dir);

  plan_tests(6);
  test_reach(map, 0, 0.1, 0);
  test_reach(map, 0, 0.1, 750);
  test_reach(map, 0, 0.1, 500);
  test_reach(map, 0, 0.1, 250);

  return exit_status();
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
