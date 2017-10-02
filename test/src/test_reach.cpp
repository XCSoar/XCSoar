/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/
#include <iostream>
#include <fstream>
#include "Printing.hpp"
#define DO_PRINT
#include "TestUtil.hpp"
#include "Route/TerrainRoute.hpp"
#include "Engine/Route/ReachResult.hpp"
#include "Terrain/RasterMap.hpp"
#include "Terrain/Loader.hpp"
#include "OS/ConvertPathName.hpp"
#include "Compatibility/path.h"
#include "GlideSolvers/GlideSettings.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Geo/SpeedVector.hpp"
#include "Operation/Operation.hpp"
#include "OS/FileUtil.hpp"

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

  bool retval= true;

  int horigin = map.GetHeight(origin).GetValueOr0() + 1000;
  AGeoPoint aorigin(origin, horigin);

  retval = route.SolveReachTerrain(aorigin, config, INT_MAX);
  ok(retval, "reach terrain", 0);
  PrintHelper::print_reach_terrain_tree(route);

  retval = route.SolveReachWorking(aorigin, config, INT_MAX);
  ok(retval, "reach working", 0);
  PrintHelper::print_reach_working_tree(route);

  {
    Directory::Create(Path(_T("output/results")));
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
        ReachResult reach;
        route.FindPositiveArrival(adest, reach);
        if ((i % 5 == 0) && (j % 5 == 0)) {
          AGeoPoint ao2(x, h + 1000);
          route.SolveReachTerrain(ao2, config, INT_MAX);
        }
        fout << x.longitude.Degrees() << " "
             << x.latitude.Degrees() << " "
             << h << " " << (int)reach.terrain << "\n";
      }
      fout << "\n";
    }
    fout << "\n";
  }

  //  double pd = map.PixelDistance(origin, 1);
  //  printf("# pixel size %g\n", (double)pd);
}

int main(int argc, char** argv) {
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

  NullOperationEnvironment operation;
  if (!LoadTerrainOverview(dir, map.GetTileCache(),
                           operation)) {
    fprintf(stderr, "failed to load map\n");
    return EXIT_FAILURE;
  }

  map.UpdateProjection();

  SharedMutex mutex;
  do {
    UpdateTerrainTiles(dir, map.GetTileCache(), mutex,
                       map.GetProjection(),
                       map.GetMapCenter(), 50000);
  } while (map.IsDirty());
  zzip_dir_close(dir);

  plan_tests(8);
  test_reach(map, 0, 0.1, 0);
  test_reach(map, 0, 0.1, 750);
  test_reach(map, 0, 0.1, 500);
  test_reach(map, 0, 0.1, 250);

  return exit_status();
}

