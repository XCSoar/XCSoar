#include "Waypoints.hpp"
#include "BaseTask/TaskProjection.h"
#include <deque>
#include <vector>
#include <fstream>

void 
Waypoints::fill_default()
{
  std::ofstream fin("res-wp-in.txt");
  WAYPOINT wp[6];
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

  task_projection.reset(wp[0].Location);
  for (unsigned i=0; i<5; i++) {
    task_projection.scan_location(wp[i].Location);
  }
  task_projection.update_fast();
  task_projection.report();

  for (unsigned i=0; i<5; i++) {
    waypoint_tree.insert(wp[i]);
  }

  for (unsigned i=0; i<150; i++) {
    int x = rand()%1200-600;
    int y = rand()%1200-600;
    WAYPOINT ff; ff.FlatLocation.Longitude = x; ff.FlatLocation.Latitude = y;
    ff.Location = task_projection.unproject(ff.FlatLocation);
    ff.id = i+4;
    waypoint_tree.insert(ff);
    ff.print(fin, task_projection);
  }

  waypoint_tree.optimize();
}


Waypoints::WaypointTree::const_iterator 
Waypoints::find_nearest(const GEOPOINT &loc) const 
{
  FLAT_GEOPOINT floc = task_projection.project(loc);
  WAYPOINT bb_target; bb_target.FlatLocation = floc;
  std::pair<WaypointTree::const_iterator, double> 
    found = waypoint_tree.find_nearest(bb_target);
  return found.first;
}


Waypoints::WaypointTree::const_iterator
Waypoints::find_id(const unsigned id) const
{
  WAYPOINT bb_target; bb_target.id = id;
  WaypointTree::const_iterator found = waypoint_tree.find_exact(bb_target);
  return found;
}


std::vector< WAYPOINT >
Waypoints::find_within_range(const GEOPOINT &loc, 
                             const unsigned &range) const
{
  FLAT_GEOPOINT floc = task_projection.project(loc);
  WAYPOINT bb_target; bb_target.FlatLocation = floc;
  
  std::vector< WAYPOINT > vectors;
  waypoint_tree.find_within_range(bb_target, range, 
                                  std::back_inserter(vectors));
  return vectors;
}


std::vector< WAYPOINT >
Waypoints::find_within_range_circle(const GEOPOINT &loc, 
                                    const unsigned &range) const
{
  std::vector < WAYPOINT > vectors = find_within_range(loc, range);
  FLAT_GEOPOINT floc = task_projection.project(loc);

  for (std::vector< WAYPOINT >::iterator v=vectors.begin();
       v != vectors.end(); ) {      
    if ((*v).FlatLocation.distance_to(floc)> range) {
      vectors.erase(v);
    } else {
      v++;
    }
  }
  return vectors;
}


Waypoints::WaypointTree::const_iterator
Waypoints::begin() const
{
  return waypoint_tree.begin();
}

Waypoints::WaypointTree::const_iterator
Waypoints::end() const
{
  return waypoint_tree.end();
}
