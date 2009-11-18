#ifndef WaypointS_HPP
#define WaypointS_HPP

#include <kdtree++/kdtree.hpp>
#include "Waypoint.hpp"
#ifdef DO_PRINT
#include <iostream>
#endif
#include <deque>

#include "Navigation/TaskProjection.hpp"

class WaypointVisitor;

class Waypoints {
public:
/** 
 * Constructor.  Task projection is updated after call to optimise().
 * As waypoints are added they are stored temporarily before applying
 * projection so that the bounds of the projection may be obtained.
 * 
 * See below for usage notes --- further work is required.
 * 
 */
  Waypoints();

  typedef KDTree::KDTree<2, 
                         Waypoint, 
                         Waypoint::kd_get_location
                         > WaypointTree;

/** 
 * Looks up nearest waypoint to the search location.
 * Performs search according to flat-earth internal representation,
 * so is approximate.
 * 
 * @param loc Location from which to search
 * 
 * @return Iterator to absolute nearest waypoint 
 */
  WaypointTree::const_iterator find_nearest(const GEOPOINT &loc) const;

/** 
 * Look up waypoint by ID.
 * 
 * @param id Id of waypoint to find in internal tree
 * 
 * @return Iterator to matching waypoint (or end if not found)
 */
  WaypointTree::const_iterator find_id(const unsigned id) const;

/** 
 * Find waypoints within approximate range (square range box)
 * to search location.  Possible use by screen display functions.
 * 
 * @param loc Location from which to search
 * @param range Distance in meters of search radius
 * 
 * @return Vector of waypoints within square range
 */
  std::vector< Waypoint >
    find_within_range(const GEOPOINT &loc, const double range) const;

/** 
 * Call visitor function on waypoints within approximate range
 * (square range box) to search location.  Possible use by screen display
 * functions.
 * 
 * @param loc Location from which to search
 * @param range Distance in meters of search radius
 * @param visitor Visitor to be called on waypoints within range
 */
  void visit_within_range(const GEOPOINT &loc, const double range,
                          WaypointVisitor& visitor) const;

/** 
 * Find waypoints within specified flat-earth distance to
 * search location.
 * 
 * @param loc Location from which to search
 * @param range Distance in meters of search radius
 * 
 * @return Vector of waypoints within circular range
 */
  std::vector< Waypoint >
    find_within_range_circle(const GEOPOINT &loc, const double range) const;

/** 
 * Add waypoint to internal store.  Internal copy is made.
 * optimise() must be called after inserting waypoints prior to
 * performing any queries, but can be done in batches.
 * 
 * @param wp Waypoint to add to internal store
 */
  void insert(const Waypoint& wp);

/** 
 * Optimise the internal search tree after adding/removing elements.
 * Also performs projection to flat earth for new elements.
 * This updates the task_projection.
 * 
 * Note: currently this code doesn't check for task projections
 * being modified from multiple calls to optimise() so it should
 * only be called once (until this is fixed).
 */
  void optimise();

/** 
 * Access first waypoint in store, for use in iterators.
 * 
 * @return First waypoint in store
 */
  WaypointTree::const_iterator begin() const;

/** 
 * Access end waypoint in store, for use in iterators as end point.
 * 
 * @return End waypoint in store
 */
  WaypointTree::const_iterator end() const;

/** 
 * Clear the waypoint store
 * 
 */
  void clear();

private:
  WaypointTree waypoint_tree;
  TaskProjection task_projection;

  std::deque< Waypoint > tmp_wps;

  /** @link dependency */
  /*#  Waypoint lnkWaypoint; */
};

#endif
