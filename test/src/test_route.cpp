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

#include "Math/FastMath.h"
#include "Printing.hpp"
#define DO_PRINT
#include "harness_flight.hpp"
#include "harness_airspace.hpp"
#include "Route/AirspaceRoute.hpp"
#include "Navigation/SpeedVector.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Terrain/RasterMap.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"
#include "Operation.hpp"

extern Airspaces *airspaces;

#define NUM_SOL 15

static
bool test_route(const unsigned n_airspaces, const RasterMap& map)
{
  airspaces = new Airspaces;
  setup_airspaces(*airspaces, map.GetMapCenter(), n_airspaces);

  {
    std::ofstream fout ("results/terrain.txt");
    unsigned nx = 100;
    unsigned ny = 100;
    GeoPoint origin(map.GetMapCenter());
    for (unsigned i=0; i< nx; ++i) {
      for (unsigned j=0; j< ny; ++j) {
        fixed fx = (fixed)i/(nx-1)*fixed(2.0)-fixed_one;
        fixed fy = (fixed)j/(ny-1)*fixed(2.0)-fixed_one;
        GeoPoint x(origin.Longitude+Angle::degrees(fixed(0.2)+fixed(0.7)*fx),
                   origin.Latitude+Angle::degrees(fixed(0.9)*fy));
        short h = map.GetInterpolatedHeight(x);
        fout << x.Longitude.value_degrees() << " " << x.Latitude.value_degrees() << " " << h << "\n";
      }
      fout << "\n";
    }
    fout << "\n";
  }

  { // local scope, see what happens when we go out of scope
    GeoPoint p_start(Angle::degrees(fixed(-0.3)),Angle::degrees(fixed(0.0))); p_start += map.GetMapCenter();
    GeoPoint p_dest(Angle::degrees(fixed(0.8)),Angle::degrees(fixed(-0.7))); p_dest += map.GetMapCenter();
    AGeoPoint loc_start(p_start,map.GetHeight(p_start)+100);
    AGeoPoint loc_end(p_dest,map.GetHeight(p_dest)+100);
    AIRCRAFT_STATE state;
    GlidePolar glide_polar(fixed(0.1));
    AirspaceAircraftPerformanceGlide perf(glide_polar);

    GeoVector vec(loc_start, loc_end);
    fixed range = fixed(10000)+ vec.Distance / 2;

    state.Location = loc_start;
    state.NavAltitude = fixed(loc_start.altitude);
    state.AirspaceAltitude = fixed(loc_start.altitude);

    {
      Airspaces as_route(*airspaces, false);
      // dummy

      // real one, see if items changed
      as_route.synchronise_in_range(*airspaces, vec.mid_point(loc_start), range);
      if (verbose)
        printf("# route airspace size %d\n", as_route.size());
      int size_1 = as_route.size();

      as_route.synchronise_in_range(*airspaces, vec.mid_point(loc_start), fixed_one);
      if (verbose)
        printf("# route airspace size %d\n", as_route.size());
      int size_2 = as_route.size();

      ok(size_2<size_1,"shrink as",0);

      // go back
      as_route.synchronise_in_range(*airspaces, vec.mid_point(loc_end), range);
      if (verbose)
        printf("# route airspace size %d\n", as_route.size());
      int size_3 = as_route.size();

      ok(size_3>=size_2,"grow as",0);

      // and again
      as_route.synchronise_in_range(*airspaces, vec.mid_point(loc_start), range);
      if (verbose)
        printf("# route airspace size %d\n", as_route.size());
      int size_4 = as_route.size();

      ok(size_4>=size_3,"grow as",0);

      scan_airspaces(state, as_route, perf, true, loc_end);
    }

    // try the solver
    SpeedVector wind(Angle::degrees(fixed(0)), fixed(0.0));
    GlidePolar polar(fixed_one);

    AirspaceRoute route(polar, wind, *airspaces);
    route.set_terrain(&map);
    RoutePlannerConfig config;
    config.mode = RoutePlannerConfig::rpBoth;

    bool sol = false;
    for (int i=0; i<NUM_SOL; i++) {
      loc_end.Latitude+= Angle::degrees(fixed(0.1));
      loc_end.altitude = map.GetHeight(loc_end)+100;
      route.synchronise(*airspaces, loc_start, loc_end);
      if (route.solve(loc_start, loc_end, config)) {
        sol = true;
        if (verbose) {
          PrintHelper::print_route(route);
        }
      } else {
        if (verbose) {
          printf("# fail\n");
        }
        sol = false;
      }
      char buffer[80];
      sprintf(buffer,"route %d solution", i);
      ok(sol, buffer, 0);
    }
  }

  delete airspaces; airspaces = NULL;
  return true;
}


int main(int argc, char** argv)
{
  // default arguments
  autopilot_parms.ideal();

  if (!parse_args(argc,argv)) {
    return 0;
  }

  const char hc_path[] = "tmp/terrain";

  TCHAR jp2_path[4096];
  _tcscpy(jp2_path, PathName(hc_path));
  _tcscat(jp2_path, _T(DIR_SEPARATOR_S) _T("terrain.jp2"));

  TCHAR j2w_path[4096];
  _tcscpy(j2w_path, PathName(hc_path));
  _tcscat(j2w_path, _T(DIR_SEPARATOR_S) _T("terrain.j2w"));

  OperationEnvironment operation;
  RasterMap map(jp2_path, j2w_path, NULL, operation);
  do {
    map.SetViewCenter(map.GetMapCenter(), fixed(100000));
  } while (map.IsDirty());

  plan_tests(4+NUM_SOL);
  ok(test_route(28, map),"route 28",0);
  return exit_status();
}
