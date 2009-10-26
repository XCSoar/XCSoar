#ifndef WAYPOINTS_HPP
#define WAYPOINTS_HPP

#include <kdtree++/kdtree.hpp>
#include "Waypoint.hpp"
#include <iostream>
#include <deque>

class Waypoints {
public:
  Waypoints(TaskProjection& _task_projection);

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

  void optimise();
  void insert(const WAYPOINT& wp);

private:
  WaypointTree waypoint_tree;
  TaskProjection& task_projection;

  std::deque< WAYPOINT > tmp_wps;

  /** @link dependency */
  /*#  WAYPOINT lnkWAYPOINT; */
public:
  WaypointTree::const_iterator begin() const;
  WaypointTree::const_iterator end() const;
};

#endif
