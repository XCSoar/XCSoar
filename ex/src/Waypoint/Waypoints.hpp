#ifndef WaypointS_HPP
#define WaypointS_HPP

#include <kdtree++/kdtree.hpp>
#include "Waypoint.hpp"
#ifdef DO_PRINT
#include <iostream>
#endif
#include <deque>

class WaypointVisitor;

class Waypoints {
public:
  Waypoints(TaskProjection& _task_projection);

  typedef KDTree::KDTree<2, 
                         Waypoint, 
                         Waypoint::kd_get_location
                         > WaypointTree;

  WaypointTree::const_iterator find_nearest(const GEOPOINT &loc) const;

  WaypointTree::const_iterator find_id(const unsigned id) const;

  std::vector< Waypoint >
    find_within_range(const GEOPOINT &loc, const double range) const;

  std::vector< Waypoint >
    find_within_range_circle(const GEOPOINT &loc, const double range) const;

  void optimise();
  void insert(const Waypoint& wp);

  void visit_within_range(const GEOPOINT &loc, const double range,
                          WaypointVisitor& visitor) const;

  const TaskProjection &get_task_projection() const {
    return task_projection;
  }
private:
  WaypointTree waypoint_tree;
  TaskProjection& task_projection;

  std::deque< Waypoint > tmp_wps;

  /** @link dependency */
  /*#  Waypoint lnkWaypoint; */
public:
  WaypointTree::const_iterator begin() const;
  WaypointTree::const_iterator end() const;
};

#endif
