#include "Waypoints.hpp"
#include "BaseTask/TaskProjection.h"
#include <vector>
#include <fstream>


unsigned n_queries = 0;

Waypoints::Waypoints(TaskProjection& _task_projection):
  task_projection(_task_projection)
{
}

void
Waypoints::optimise()
{
  std::ofstream fin("res-wp-in.txt");

  task_projection.update_fast();

  while (!tmp_wps.empty()) {
    WAYPOINT w = (tmp_wps.front());
    w.FlatLocation = task_projection.project(w.Location);
    waypoint_tree.insert(w);
    tmp_wps.pop_front();
    w.print(fin, task_projection);
  }

  waypoint_tree.optimize();
}

void
Waypoints::insert(const WAYPOINT& wp)
{
  if (!tmp_wps.size() && !waypoint_tree.size()) {
    task_projection.reset(wp.Location);
  }
  task_projection.scan_location(wp.Location);

  tmp_wps.push_back(wp);

  // TODO: if range changed, need to re-pack waypoints
  // will have to remove all from the list, recalculate projections,
  // then add them again!
  // (can just insert() them all, then clear the tree, then run optimise()
}



Waypoints::WaypointTree::const_iterator 
Waypoints::find_nearest(const GEOPOINT &loc) const 
{
  FLAT_GEOPOINT floc = task_projection.project(loc);
  WAYPOINT bb_target; bb_target.FlatLocation = floc;
  std::pair<WaypointTree::const_iterator, double> 
    found = waypoint_tree.find_nearest(bb_target);

  n_queries++;

  return found.first;
}


Waypoints::WaypointTree::const_iterator
Waypoints::find_id(const unsigned id) const
{
  WAYPOINT bb_target; bb_target.id = id;
  WaypointTree::const_iterator found = waypoint_tree.find_exact(bb_target);

  n_queries++;

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
  n_queries++;

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
