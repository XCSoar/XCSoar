/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include "OS/PathName.hpp"
#include "Compatibility/path.h"
#include "GlideSolvers/GlidePolar.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Navigation/SpeedVector.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Operation.hpp"

static void test_troute(const RasterMap& map, fixed mwind, fixed mc, short ceiling)
{
  GlidePolar polar(mc);
  SpeedVector wind(Angle::degrees(fixed(0)), mwind);
  TerrainRoute route;
  route.UpdatePolar(polar, polar, wind);
  route.SetTerrain(&map);

  GeoPoint origin(map.GetMapCenter());

  fixed pd = map.pixel_distance(origin, 1);
  printf("# pixel size %g\n", (double)pd);

  bool retval= true;

  {
    std::ofstream fout ("results/terrain.txt");
    unsigned nx = 100;
    unsigned ny = 100;
    for (unsigned i=0; i< nx; ++i) {
      for (unsigned j=0; j< ny; ++j) {
        fixed fx = (fixed)i/(nx-1)*fixed_two-fixed_one;
        fixed fy = (fixed)j/(ny-1)*fixed_two-fixed_one;
        GeoPoint x(origin.Longitude+Angle::degrees(fixed(0.6)*fx),
                   origin.Latitude+Angle::degrees(fixed(0.4)*fy));
        short h = map.GetInterpolatedHeight(x);
        fout << x.Longitude.value_degrees() << " " << x.Latitude.value_degrees() << " " << h << "\n";
      }
      fout << "\n";
    }
    fout << "\n";
  }

  RoutePlannerConfig config;
  config.mode = RoutePlannerConfig::rpBoth;

  unsigned i=0;
  for (fixed ang=fixed_zero; ang< fixed_two_pi; ang+= fixed_quarter_pi*fixed_half) {
    GeoPoint dest = GeoVector(fixed(40000.0), Angle::radians(ang)).end_point(origin);

    short hdest = map.GetHeight(dest)+100;

    retval = route.Solve(AGeoPoint(origin, map.GetHeight(origin)+100),
                         AGeoPoint(dest, positive(mc)? hdest: std::max(hdest, (short)3200)),
                         config, ceiling);
    char buffer[80];
    sprintf(buffer,"terrain route solve, dir=%g, wind=%g, mc=%g ceiling=%d",
            (double)ang, (double)mwind, (double)mc, ceiling);
    ok(retval, buffer, 0);
    PrintHelper::print_route(route);
    i++;
  }

  // polar.SetMC(fixed_zero);
  // route.UpdatePolar(polar, wind);
}

int main(int argc, char** argv) {

  const char hc_path[] = "tmp/terrain";
  const char *map_path;
  if ((argc<2) || !strlen(argv[0])) {
    map_path = hc_path;
  } else {
    map_path = argv[0];
  }

  TCHAR jp2_path[4096];
  _tcscpy(jp2_path, PathName(map_path));
  _tcscat(jp2_path, _T(DIR_SEPARATOR_S) _T("terrain.jp2"));

  TCHAR j2w_path[4096];
  _tcscpy(j2w_path, PathName(map_path));
  _tcscat(j2w_path, _T(DIR_SEPARATOR_S) _T("terrain.j2w"));

  NullOperationEnvironment operation;
  RasterMap map(jp2_path, j2w_path, NULL, operation);
  do {
    map.SetViewCenter(map.GetMapCenter(), fixed(100000));
  } while (map.IsDirty());

  plan_tests(16*3);
  test_troute(map, fixed_zero, fixed(0.1), 10000);
  test_troute(map, fixed_zero, fixed_zero, 10000);
  test_troute(map, fixed(5.0), fixed_one, 10000);

  return exit_status();
}

