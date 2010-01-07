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

bool test_wp(const unsigned n, std::ostream &fo) {
  Waypoints waypoints;
  setup_waypoints(waypoints,n);

  AIRCRAFT_STATE state;

  print_queries(0, fo);

  for (unsigned i=0; i<n_test; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    state.Location.Longitude = x/1000.0; 
    state.Location.Latitude = y/1000.0;

    WaypointVisitorPrint wvp;
    waypoints.visit_within_radius(state.Location, fixed(50000.0), wvp);
  }
  print_queries(n, fo);
  fo.flush();
  return true;
}


bool test_as(const unsigned n, std::ostream &fo) {
  AIRCRAFT_STATE state;

  Airspaces airspaces;
  setup_airspaces(airspaces,n);

  print_queries(0, fo);

  for (unsigned i=0; i<n_test; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    state.Location.Longitude = x/1000.0; 
    state.Location.Latitude = y/1000.0;
    airspaces.find_inside(state);
  }
  print_queries(n, fo);
  fo.flush();

  airspaces.scan_range(state, fixed(20000.0));

  return true;
}


int main(int argc, char** argv) {
  ::InitSineTable();

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
