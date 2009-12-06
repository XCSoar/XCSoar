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

  wp = waypoints.create(GEOPOINT(0,0));
  wp.Flags.Airport = true;
  wp.Altitude=0.25;
  waypoints.append(wp);

  wp = waypoints.create(GEOPOINT(0.0,1.0));
  wp.Flags.Airport = true;
  wp.Altitude=0.25;
  waypoints.append(wp);

  wp = waypoints.create(GEOPOINT(1.0,1.0));
  wp.Name = "Hello";
  wp.Flags.Airport = true;
  wp.Altitude=0.5;
  waypoints.append(wp);

  wp = waypoints.create(GEOPOINT(0.8,0.5));
  wp.Name = "Unk";
  wp.Flags.Airport = true;
  wp.Altitude=0.25;
  waypoints.append(wp);

  wp = waypoints.create(GEOPOINT(1.0,0.0));
  wp.Flags.Airport = true;
  wp.Altitude=0.25;
  waypoints.append(wp);

  wp = waypoints.create(GEOPOINT(0.0,0.23));
  wp.Flags.Airport = true;
  wp.Altitude=0.25;
  waypoints.append(wp);

  for (unsigned i=0; i<(unsigned)std::max((int)n-6,0); i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    double z = rand()% std::max(terrain_height,1);
    wp = waypoints.create(GEOPOINT(x/1000.0,y/1000.0));
    wp.Flags.Airport = false;
    wp.Altitude = z;
    waypoints.append(wp);
  }
  waypoints.optimise();

  if (verbose) {
    std::ofstream fin("results/res-wp-in.txt");
    for (unsigned i=1; i<=waypoints.size(); i++) {
      Waypoints::WaypointTree::const_iterator it = waypoints.find_id(i);
      if (it != waypoints.end()) {
#ifdef DO_PRINT
        fin << it->get_waypoint();
#endif
      }
    }
  }
  return true;
}

