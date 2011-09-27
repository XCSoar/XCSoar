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

static void test_reach(const RasterMap& map, fixed mwind, fixed mc)
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

  short horigin = map.GetHeight(origin)+1000;
  AGeoPoint aorigin(origin, RoughAltitude(horigin));

  retval = route.SolveReach(aorigin);

  ok(retval, "reach solve", 0);

  PrintHelper::print_reach_tree(route);

  GeoPoint dest(origin.longitude-Angle::degrees(fixed(0.02)),
                origin.latitude-Angle::degrees(fixed(0.02)));

  {
    std::ofstream fout ("results/terrain.txt");
    unsigned nx = 100;
    unsigned ny = 100;
    for (unsigned i=0; i< nx; ++i) {
      for (unsigned j=0; j< ny; ++j) {
        fixed fx = (fixed)i/(nx-1)*fixed_two-fixed_one;
        fixed fy = (fixed)j/(ny-1)*fixed_two-fixed_one;
        GeoPoint x(origin.longitude+Angle::degrees(fixed(0.6)*fx),
                   origin.latitude+Angle::degrees(fixed(0.6)*fy));
        short h = map.GetInterpolatedHeight(x);
        AGeoPoint adest(x, RoughAltitude(h));
        RoughAltitude ha, hd;
        route.FindPositiveArrival(adest, ha, hd);
        if ((i % 5 == 0) && (j % 5 == 0)) {
          AGeoPoint ao2(x, RoughAltitude(h + 1000));
          route.SolveReach(ao2);
        }
        fout << x.longitude.value_degrees() << " "
             << x.latitude.value_degrees() << " "
             << h << " " << (int)ha << "\n";
      }
      fout << "\n";
    }
    fout << "\n";
  }
}

int main(int argc, char** argv) {

  const char hc_path[] = "tmp/terrain";
  const char *map_path;
  if ((argc<2) || !strlen(argv[1])) {
    map_path = hc_path;
  } else {
    map_path = argv[1];
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

  plan_tests(1);
  test_reach(map, fixed_zero, fixed(0.1));

  return exit_status();
}

