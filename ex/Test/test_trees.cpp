#include "harness_waypoints.hpp"
#include "harness_airspace.hpp"
#include "test_debug.hpp"

#define n_test 500

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
    std::vector < Waypoint > approx_waypoints = 
      waypoints.find_within_range_circle(state.Location, 50000.0);
//    printf("s %d\n",(int)approx_waypoints.size());
    (void)approx_waypoints.size();
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
  return true;
}


int main() {
  interactive = false;
  ::InitSineTable();

  std::ofstream fw("results/res-tree-wp.txt");

  fw << "# test waypoint tree\n";
  for (double i=10; i<=4000; i*= 1.1) {
    test_wp((int)i,fw);
  }
  fw << "\n";

  std::ofstream fa("results/res-tree-as.txt");

  fa << "# test airspace tree\n";
  for (double i=10; i<=4000; i*= 1.1) {
    test_as((int)i,fa);
  }
  fa << "\n";
  return 0;
}
