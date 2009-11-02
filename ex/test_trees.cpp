#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Navigation/Waypoints.hpp"
#include "Math/FastMath.h"
#include "Task/TaskManager.hpp"
#include "Task/TaskEvents.hpp"
#include "Task/Tasks/TaskPoints/FAISectorStartPoint.hpp"
#include "Task/Tasks/TaskPoints/FAISectorFinishPoint.hpp"
#include "Task/Tasks/TaskPoints/FAISectorASTPoint.hpp"
#include "Task/Tasks/TaskPoints/FAICylinderASTPoint.hpp"
#include "Task/Tasks/TaskPoints/CylinderAATPoint.hpp"

#include <fstream>

std::ofstream fout("res-trees.txt");

#ifdef INSTRUMENT_TASK
extern unsigned count_intersections;
extern unsigned n_queries;
#endif

const unsigned n_test = 2000;

void print_queries(unsigned n) {
#ifdef INSTRUMENT_TASK
  fout << n << " " << count_intersections/n_queries << "\n";
  count_intersections = 0;
  n_queries = 0;
#endif
}

void
test_wp(const unsigned n) 
{
  GEOPOINT start; start.Longitude = 0.5; start.Latitude = 0.5;
  TaskProjection task_projection(start);
  Waypoints waypoints(task_projection);
  AIRCRAFT_STATE state;

#ifdef INSTRUMENT_TASK
  count_intersections = 0;
  n_queries = 0;
#endif

  for (unsigned i=0; i<n; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    Waypoint ff; 
    ff.Location.Longitude = x/1000.0; 
    ff.Location.Latitude = y/1000.0;
    ff.id = i+4;
    waypoints.insert(ff);
  }
  waypoints.optimise();

  for (unsigned i=0; i<n_test; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    state.Location.Longitude = x/1000.0; 
    state.Location.Latitude = y/1000.0;
    std::vector < Waypoint > approx_waypoints = 
      waypoints.find_within_range_circle(state.Location, 20);
    (void)approx_waypoints.size();
  }
  print_queries(n);
}


void
test_as(const unsigned n, TaskProjection &task_projection)
{
  AIRCRAFT_STATE state;
  Airspaces airspaces(task_projection);

#ifdef INSTRUMENT_TASK
  count_intersections = 0;
  n_queries = 0;
#endif

  for (unsigned i=0; i<n; i++) {
    AbstractAirspace* as;
    if (rand()%3==0) {
      GEOPOINT c;
      c.Longitude = (rand()%1200-600)/1000.0+0.5;
      c.Latitude = (rand()%1200-600)/1000.0+0.5;
      double radius = 10000.0*(0.2+(rand()%12)/12.0);
      as = new AirspaceCircle(c,radius);
    } else {
      as = new AirspacePolygon(task_projection);
    }
    airspaces.insert(*as);
  }
  airspaces.optimise();

  for (unsigned i=0; i<n_test; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    state.Location.Longitude = x/1000.0; 
    state.Location.Latitude = y/1000.0;
    airspaces.find_inside(state);
  }
  print_queries(n);
}


int main() {
  GEOPOINT start; start.Longitude = 0.5; start.Latitude = 0.5;

  ::InitSineTable();

  ////////////////////////////////////////////////////////////////

  TaskProjection task_projection(start);

  ////////////////////////// WaypointS //////
  Waypoints waypoints(task_projection);

  Waypoint wp[6];
  wp[0].id = 0;
  wp[0].Location.Longitude=0;
  wp[0].Location.Latitude=0;
  wp[0].Altitude=0.25;
  wp[1].id = 1;
  wp[1].Location.Longitude=0;
  wp[1].Location.Latitude=1.0;
  wp[1].Altitude=0.25;
  wp[2].id = 2;
  wp[2].Location.Longitude=1.0;
  wp[2].Location.Latitude=1.0;
  wp[2].Altitude=0.5;
  wp[3].id = 3;
  wp[3].Location.Longitude=0.8;
  wp[3].Location.Latitude=0.5;
  wp[3].Altitude=0.25;
  wp[4].id = 4;
  wp[4].Location.Longitude=1.0;
  wp[4].Location.Latitude=0;
  wp[4].Altitude=0.25;

  for (unsigned i=0; i<5; i++) {
    waypoints.insert(wp[i]);
  }

  for (unsigned i=0; i<150; i++) {
    int x = rand()%1200-100;
    int y = rand()%1200-100;
    Waypoint ff; 
    ff.Location.Longitude = x/1000.0; 
    ff.Location.Latitude = y/1000.0;
    ff.id = i+4;
    waypoints.insert(ff);
  }
  waypoints.optimise();

#ifdef DO_PRINT
  std::cout << task_projection;
#endif

  fout << "# test waypoint tree\n";
  for (double i=10; i<=10000; i*= 1.1) {
    test_wp(i);
  }
  fout << "\n";

  ////////////////////////// AIRSPACES //////

  fout << "# test airspace tree\n";
  for (double i=10; i<=10000; i*= 1.1) {
    test_as(i,task_projection);
  }
  fout << "\n";
  return 0;
}
