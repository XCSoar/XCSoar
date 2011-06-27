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

#include "harness_waypoints.hpp"
#include "harness_airspace.hpp"
#include "test_debug.hpp"

#define n_test 500

#include "Waypoint/WaypointVisitor.hpp"

class WaypointVisitorPrint: public WaypointVisitor {
public:
  WaypointVisitorPrint():count(0) {};

  virtual void Visit(const Waypoint& wp) {
    count++;
  }
  unsigned count;
};

static bool
test_wp(const unsigned n, std::ostream &fo)
{
  Waypoints waypoints;
  setup_waypoints(waypoints,n);

  AIRCRAFT_STATE state;

  print_queries(0, fo);

  for (unsigned i=0; i<n_test; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    state.Location.Longitude = Angle::degrees(fixed(x/1000.0)); 
    state.Location.Latitude = Angle::degrees(fixed(y/1000.0));

    WaypointVisitorPrint wvp;
    waypoints.visit_within_range(state.Location, fixed(50000.0), wvp);
  }
  print_queries(n, fo);
  fo.flush();
  return true;
}

static bool
test_as(const unsigned n, std::ostream &fo)
{
  AIRCRAFT_STATE state;

  Airspaces airspaces;
  setup_airspaces(airspaces,GeoPoint(Angle::native(fixed_zero), Angle::native(fixed_zero)), n);

  print_queries(0, fo);

  for (unsigned i=0; i<n_test; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    state.Location.Longitude = Angle::degrees(fixed(x/1000.0)); 
    state.Location.Latitude = Angle::degrees(fixed(y/1000.0));
    const AirspacesInterface::AirspaceVector vc = airspaces.find_inside(state);
  }
  print_queries(n, fo);
  fo.flush();

  const AirspacesInterface::AirspaceVector vc =
    airspaces.scan_range(state.Location, fixed(20000.0));

  return true;
}


int main(int argc, char** argv) {
  if (!parse_args(argc,argv)) {
    return 0;
  }

  std::ofstream fw("results/res-tree-wp.txt");

  plan_tests(2);

  bool fine = true;
  fw << "# test waypoint tree\n";
  for (double i=10; i<=4000; i*= 1.1) {
    fine &= test_wp((int)i,fw);
  }
  fw << "\n";
  ok(fine,"waypoint tree",0);

  std::ofstream fa("results/res-tree-as.txt");

  fine = true;
  fa << "# test airspace tree\n";
  for (double i=10; i<=4000; i*= 1.1) {
    fine &= test_as((int)i,fa);
  }
  fa << "\n";
  ok(fine,"airspace tree",0);

  return exit_status();
}
