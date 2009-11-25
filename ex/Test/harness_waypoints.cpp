#include "harness_waypoints.hpp"
#include "test_debug.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

/** 
 * Initialises waypoints with random and non-random waypoints
 * for testing
 *
 * @param waypoints waypoints class to add waypoints to
 */
bool setup_waypoints(Waypoints &waypoints, const unsigned n) 
{

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

  wp.id++;
  wp.Location.Longitude=0;
  wp.Location.Latitude=0.23;
  wp.Altitude=0.25;
  waypoints.insert(wp);

  for (unsigned i=0; i<(unsigned)std::max((int)n-6,0); i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    double z = rand()% std::max(terrain_height,1);
    wp.id++;
    wp.Location.Longitude = x/1000.0; 
    wp.Location.Latitude = y/1000.0;
    wp.Altitude = z;
    waypoints.insert(wp);
  }
  waypoints.optimise();

  if (verbose) {
    std::ofstream fin("results/res-wp-in.txt");
    for (unsigned i=1; i<=waypoints.size(); i++) {
      Waypoints::WaypointTree::const_iterator it = waypoints.find_id(i);
      if (it != waypoints.end()) {
#ifdef DO_PRINT
        fin << *it;
#endif
      }
    }
  }
  return true;
}

