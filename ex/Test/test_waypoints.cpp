#include "test_waypoints.hpp"
#include <fstream>
#include <iostream>

#include "Waypoint/WaypointVisitor.hpp"

class WaypointVisitorPrint: public WaypointVisitor {
public:
  WaypointVisitorPrint():count(0) {};

  virtual void Visit(const Waypoint& wp) {
    printf("# visiting wp %d\n", wp.id);
    count++;
  }
  void summary() {
    printf("# there are %d wps found\n",count);
  }
  unsigned count;
};


/** 
 * Initialises waypoints with random and non-random waypoints
 * for testing
 *
 * @param waypoints waypoints class to add waypoints to
 */
void setup_waypoints(Waypoints &waypoints) {
#ifdef DO_PRINT
  std::ofstream fin("results/res-wp-in.txt");
#endif

  Waypoint wp;

  wp.id = 1;
  wp.Location.Longitude=0;
  wp.Location.Latitude=0;
  wp.Altitude=0.25;
  waypoints.insert(wp);

  wp.id++;
  wp.Location.Longitude=0;
  wp.Location.Latitude=1.0;
  wp.Altitude=0.25;
  waypoints.insert(wp);

  wp.id++;
  wp.Location.Longitude=1.0;
  wp.Location.Latitude=1.0;
  wp.Altitude=0.5;
  waypoints.insert(wp);

  wp.id++;
  wp.Location.Longitude=0.8;
  wp.Location.Latitude=0.5;
  wp.Altitude=0.25;
  waypoints.insert(wp);

  wp.id++;
  wp.Location.Longitude=1.0;
  wp.Location.Latitude=0;
  wp.Altitude=0.25;
  waypoints.insert(wp);

  for (unsigned i=0; i<150; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    wp.id++;
    wp.Location.Longitude = x/1000.0; 
    wp.Location.Latitude = y/1000.0;
    waypoints.insert(wp);
  }
  waypoints.optimise();

  for (unsigned i=1; i<=waypoints.size(); i++) {
    Waypoints::WaypointTree::const_iterator it = waypoints.find_id(i);
    if (it != waypoints.end()) {
#ifdef DO_PRINT
      fin << *it;
#endif
    }
  }

  const Waypoint *r = waypoints.lookup_id(3);
  if (r) {
    WaypointVisitorPrint v;
    waypoints.visit_within_range(r->Location, 100, v);
    v.summary();
  }
}

