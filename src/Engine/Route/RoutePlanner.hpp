// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RoutePolars.hpp"
#include "Route.hpp"
#include "RouteLink.hpp"
#include "AStar.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Geo/SearchPointVector.hpp"

#include <utility>
#include <unordered_set>

#include <limits.h>

class GlidePolar;

/**
 * RoutePlanner is an abstract class for planning paths (routes) through
 * an arbitrary environment, avoiding obstacles of different types.
 *
 * Paths take into account the origin and destination altitudes and use
 * an approximate form of the aircraft performance model in path planning,
 * such that the solution path is minimum time.  Effect of wind and glide polar
 * variations (mc value, bugs, ballast etc) are accounted for.
 *
 * Where climbs are necessary to reach the destination, the climbs are assumed
 * to occur at the start of the route.  Solutions are allowed in which the
 * aircraft arrives at the destination higher than specified.
 *
 * Climbs above the higher of the start and destination altitude are penalised
 * by a slower MC than that specified in the glide polar.  This is required to
 * prevent the optimiser from simply climbing over all mountains.
 *
 * Climbs may also be limited by an absolute ceiling if required.  This will
 * allow inversion heights etc to be incorporated.
 *
 * Climb-Cruise mode flight is assumed to be performed with no height loss and
 * course variations (in other words, a straight line).  This simplification is
 * required to reduce computational load, but also makes sense since otherwise
 * the planner would have to expect climbs exactly at particular locations,
 * which is unrealistic.
 *
 * Replanning is not performed when the origin/destination or other properties
 * have not changed.
 *
 * Failures of the solver result in the route reverting to direct flight from
 * origin to destination.
 *
 * See AirspaceRoute for an extension of RoutePlanner which avoids terrain as
 * well as airspace.
 */
class RoutePlanner {
  struct RoutePointHasher {
    constexpr std::size_t operator()(const RoutePoint &p) const noexcept {
      return p.x * std::size_t(104729) + p.y;
    }
  };

  struct RouteLinkBaseHasher {
    constexpr std::size_t operator()(const RouteLinkBase &l) const noexcept {
      RoutePointHasher p;
      return p(l.first) * std::size_t(27644437) + p(l.second);
    }
  };

  struct RouteLinkBaseEqual {
    constexpr bool operator()(const RouteLinkBase &a,
                              const RouteLinkBase &b) const noexcept {
      /* by casting the AFlatGeoPoint to FlatGeoPoint, we ignore the
         AFlatGeoPoint::altitude field which is not relevant for
         #unique_links */
      return (FlatGeoPoint)a.first == (FlatGeoPoint)b.first &&
        (FlatGeoPoint)a.second == (FlatGeoPoint)b.second;
    }
  };

protected:
  typedef std::pair<AFlatGeoPoint, AFlatGeoPoint> ClearingPair;

  /** Whether an updated solution is required */
  bool dirty;
  /** Task projection used for flat-earth representation */
  FlatProjection projection;
  /** Aircraft performance model for route calculations */
  RoutePolars rpolars_route;
  /** Minimum height scanned during solution (m) */
  int h_min;
  /** Maxmimum height scanned during solution (m) */
  int h_max;

private:
  /** A* search algorithm */
  AStar<RoutePoint, RoutePointHasher> planner{0};

  /**
   * Convex hull of search to date, used by terrain node
   * generator to prevent backtracking
   */
  SearchPointVector search_hull;

  typedef std::unordered_set<RouteLinkBase, RouteLinkBaseHasher,
                             RouteLinkBaseEqual> RouteLinkSet;

  /** Links that have been visited during solution */
  RouteLinkSet unique_links{50000};
  typedef std::queue< RouteLink> RouteLinkQueue;
  /** Link candidates to be processed for intersection tests */
  RouteLinkQueue links;

  /** Result route found by solve() method */
  Route solution_route;

  /** Origin at last call to solve() */
  AFlatGeoPoint origin_last;
  /** Destination at last call to solve() */
  AFlatGeoPoint destination_last;

protected:
  RoutePoint astar_goal;

public:
  friend class PrintHelper;

  /**
   * Constructor without initialisation of performance model and wind environment.
   * Call update_polar to update the aircraft performance model or wind estimate
   * after initialisation.
   */
  RoutePlanner() noexcept;

  /**
   * Find the optimal path.  Works in reverse time order, from the
   * origin (where you want to fly to) back to the destination (where you
   * are now).
   *
   * @param origin The start of the search (finish location)
   * @param destination The end of the search (current aircraft location)
   * @param config Control parameters for performance model constraints
   * @param h_ceiling Imposed absolute ceiling (m)
   *
   * @return True if new solution was found
   */
  bool Solve(const AGeoPoint &origin, const AGeoPoint &destination,
             const RoutePlannerConfig &config,
             int h_ceiling = INT_MAX) noexcept;

  /**
   * Retrieve current solution.  If solver failed previously,
   * direct flight from origin to destination is produced.
   */
  const Route &GetSolution() const noexcept {
    return solution_route;
  }

  /**
   * Update aircraft performance model used for path planning.
   *
   * @param polar Glide performance model used for route planning
   * @param wind Wind estimate
   * @param height_min_working Minimum working height (m)
   */
  void UpdatePolar(const GlideSettings &settings,
                   const RoutePlannerConfig &config,
                   const GlidePolar &polar,
                   const SpeedVector &wind) noexcept;

  /** Reset the optimiser as if never flown and clear temporary buffers. */
  virtual void Reset() noexcept;

protected:
  /**
   * Test whether a solution is required or the solution is trivial
   * (too short, etc.)
   *
   * @return True if solution is trivial
   */
  [[gnu::pure]]
  virtual bool IsTrivial() const noexcept {
    return !dirty;
  }

  /**
   * Add a link to candidates for search
   *
   * @param e Link to add to candidates
   */
  void AddCandidate(const RouteLink &e) noexcept;

  /**
   * Add a link to candidates for search
   * (where detailed link data is not provided)
   *
   * @param e Link to add to candidates
   */
  void AddCandidate(const RouteLinkBase &e) noexcept;

  /**
   * Attempt to add a candidate link skipping the previous
   * point to this point.
   *
   * @param p Point to find shortcut to
   */
  void AddShortcut(const RoutePoint &p) noexcept;

  /**
   * If this link is truly achievable, add or update
   * the link in the A* search algorithm.
   *
   * @param e Link to clear
   *
   * @return True if link was achievable
   */
  bool LinkCleared(const RouteLink &e) noexcept;

  /**
   * Test whether a proposed link is unique (has not been proposed for search),
   * and if so, remember that it now has been used.  This is required to prevent
   * the system from entering loops, and has a side-benefit of ensuring that
   * multiple generators of links do not waste time checking intersections that
   * have already been checked.
   *
   * @param e Link to test
   *
   * @return True if this link has not been suggested yet
   */
  bool IsSetUnique(const RouteLinkBase &e) noexcept;

protected:
  /**
   * Check whether a desired link may be flown without intersecting with
   * any obstacle.
   *
   * @param e Link to attempt
   *
   * @return true if path is clear
   */
  [[gnu::pure]]
  virtual bool IsClear(const RouteLink &e) const noexcept = 0;

  /**
   * Given a desired path e, and a clearance point, generate candidates directly
   * to the clearance point and elsewhere in attempt to avoid the obstacle.
   *
   * @param inx Clearance point from last check
   * @param e Link that was attempted
   */
  virtual void AddNearby(const RouteLink &e) noexcept = 0;

  /**
   * Hook to allow subclasses to update internal data at start of solve() call
   *
   * @param origin origin of search
   * @param destination destination of search
   */
  virtual void OnSolve(const AGeoPoint &origin,
                       const AGeoPoint &destination) noexcept;

private:
  /**
   * For a link known to not clear obstacles, generate whatever candidate edges
   * are required to attempt to avoid the obstacles or at least to continue searching.
   *
   * @param e Link to check
   */
  void AddEdges(const RouteLink &e) noexcept;

protected:
  /**
   * Test whether a candidate destination is inside the area already searched
   * or if it would extend the search area.
   *
   * @param p Candidate to test
   *
   * @return True if the candidate is outside the hull
   */
  [[gnu::pure]]
  bool IsHullExtended(const RoutePoint &p) noexcept;

private:
  /**
   * Backtrack solution from A* internal structure to construct a
   * Route.
   *
   * @param final_point Final point from search to backtrack
   * @param this_route Route to copy into
   *
   * @return Destination score (s)
   */
  unsigned FindSolution(const RoutePoint &final_point,
                        Route &this_route) const noexcept;
};
