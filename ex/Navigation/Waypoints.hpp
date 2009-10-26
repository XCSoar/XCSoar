#ifndef WAYPOINTS_HPP
#define WAYPOINTS_HPP

#include <kdtree++/kdtree.hpp>
#include "Waypoint.hpp"
#include <iostream>

class Waypoints {
public:
  Waypoints(TaskProjection& _task_projection):
    task_projection(_task_projection)
    {
      fill_default();
    };

  typedef KDTree::KDTree<2, 
                         WAYPOINT, 
                         WAYPOINT::kd_get_location
                         > WaypointTree;

  WaypointTree::const_iterator find_nearest(const GEOPOINT &loc) const;

  WaypointTree::const_iterator find_id(const unsigned id) const;

  std::vector< WAYPOINT >
    find_within_range(const GEOPOINT &loc, const unsigned &range) const;

  std::vector< WAYPOINT >
    find_within_range_circle(const GEOPOINT &loc, const unsigned &range) const;

private:
  void fill_default();

  WaypointTree waypoint_tree;
  TaskProjection& task_projection;

public:
  WaypointTree::const_iterator begin() const;
  WaypointTree::const_iterator end() const;
};

#endif
