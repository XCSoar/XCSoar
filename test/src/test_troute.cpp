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
#include "Terrain/RasterMap.hpp"
#include "Terrain/Loader.hpp"
#include "OS/ConvertPathName.hpp"
#include "Compatibility/path.h"
#include "GlideSolvers/GlideSettings.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Geo/SpeedVector.hpp"
#include "Geo/GeoVector.hpp"
#include "Operation/Operation.hpp"
#include "OS/FileUtil.hpp"

#include <zzip/zzip.h>

#include <string.h>

static void
test_troute(const RasterMap &map, double mwind, double mc, int ceiling)
{
  GlideSettings settings;
  settings.SetDefaults();
  RoutePlannerConfig config;
  config.mode = RoutePlannerConfig::Mode::BOTH;

  GlidePolar polar(mc);
  SpeedVector wind(Angle::Degrees(0), mwind);
  TerrainRoute route;
  route.UpdatePolar(settings, config, polar, polar, wind);
  route.SetTerrain(&map);

  GeoPoint origin(map.GetMapCenter());

  auto pd = map.PixelDistance(origin, 1);
  printf("# pixel size %g\n", (double)pd);

  bool retval= true;

  {
    Directory::Create(Path(_T("output/results")));
    std::ofstream fout ("output/results/terrain.txt");
    unsigned nx = 100;
    unsigned ny = 100;
    for (unsigned i=0; i< nx; ++i) {
      for (unsigned j=0; j< ny; ++j) {
        auto fx = (double)i / (nx - 1) * 2 - 1;
        auto fy = (double)j / (ny - 1) * 2 - 1;
        GeoPoint x(origin.longitude + Angle::Degrees(0.6 * fx),
                   origin.latitude + Angle::Degrees(0.4 * fy));
        TerrainHeight h = map.GetInterpolatedHeight(x);
        fout << x.longitude.Degrees() << " " << x.latitude.Degrees()
             << " " << h.GetValue() << "\n";
      }
      fout << "\n";
    }
    fout << "\n";
  }

  unsigned i=0;
  for (double ang = 0; ang < M_2PI; ang += M_PI / 8) {
    GeoPoint dest = GeoVector(40000.0, Angle::Radians(ang)).EndPoint(origin);

    int hdest = map.GetHeight(dest).GetValueOr0() + 100;

    retval = route.Solve(AGeoPoint(origin,
                                   map.GetHeight(origin).GetValueOr0() + 100),
                         AGeoPoint(dest,
                                   mc > 0
                                   ? hdest
                                   : std::max(hdest, 3200)),
                         config, ceiling);
    char buffer[128];
    sprintf(buffer,"terrain route solve, dir=%g, wind=%g, mc=%g ceiling=%d",
            (double)ang, (double)mwind, (double)mc, (int)ceiling);
    ok(retval, buffer, 0);
    PrintHelper::print_route(route);
    i++;
  }

  // polar.SetMC(0);
  // route.UpdatePolar(polar, wind);
}

int main(int argc, char** argv) {
  static const char hc_path[] = "tmp/map.xcm";
  const char *map_path;
  if ((argc<2) || !strlen(argv[0])) {
    map_path = hc_path;
  } else {
    map_path = argv[0];
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
    zzip_dir_close(dir);
    return EXIT_FAILURE;
  }

  map.UpdateProjection();

  SharedMutex mutex;
  do {
    UpdateTerrainTiles(dir, map.GetTileCache(), mutex,
                       map.GetProjection(),
                       map.GetMapCenter(), 100000);
  } while (map.IsDirty());
  zzip_dir_close(dir);

  plan_tests(16*3);
  test_troute(map, 0, 0.1, 10000);
  test_troute(map, 0, 0, 10000);
  test_troute(map, 5.0, 1, 10000);

  return exit_status();
}

