/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

#ifndef ROUTE_PLANNER_HPP
#define ROUTE_PLANNER_HPP

#include "RoutePolars.hpp"
#include "Route.hpp"
#include "RouteLink.hpp"
#include "AStar.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Geo/SearchPointVector.hpp"
#include "ReachFan.hpp"

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
 * This class has built-in support for terrain avoidance.  TerrainRoute
 * implements the pure abstract methods required for a minimal terrain avoidance
 * path planner.
 *
 * See AirspaceRoute for an extension of RoutePlanner which avoids terrain as
 * well as airspace.
 *
 * Since this class calls RasterMap functions repeatedly, rather than acquiring
 * and releasing locks each time, we assume the hookup to the main program
 * (RoutePlannerGlue) is responsible for locking the RasterMap on solve() calls.
 */
class RoutePlanner {
  struct RoutePointHasher : std::unary_function<RoutePoint, size_t> {
    gcc_const
    result_type operator()(const argument_type p) const {
      return p.x * result_type(104729) + p.y;
    }
  };

  struct RouteLinkBaseHasher : std::unary_function<RouteLinkBase, size_t> {
    gcc_const
    result_type operator()(const argument_type l) const {
      RoutePointHasher p;
      return p(l.first) * result_type(27644437) + p(l.second);
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
  /** Aircraft performance model for reach to terrain */
  RoutePolars rpolars_reach;
  /** Aircraft performance model for reach to working floor */
  RoutePolars rpolars_reach_working;
  /** Terrain raster */
  const RasterMap *terrain;
  /** Minimum height scanned during solution (m) */
  int h_min;
  /** Maxmimum height scanned during solution (m) */
  int h_max;

private:
  /** A* search algorithm */
  AStar<RoutePoint, RoutePointHasher> planner;

  /**
   * Convex hull of search to date, used by terrain node
   * generator to prevent backtracking
   */
  SearchPointVector search_hull;

  typedef std::unordered_set<RouteLinkBase, RouteLinkBaseHasher> RouteLinkSet;

  /** Links that have been visited during solution */
  RouteLinkSet unique_links;
  typedef std::queue< RouteLink> RouteLinkQueue;
  /** Link candidates to be processed for intersection tests */
  RouteLinkQueue links;

  /** Result route found by solve() method */
  Route solution_route;

  /** Origin at last call to solve() */
  AFlatGeoPoint origin_last;
  /** Destination at last call to solve() */
  AFlatGeoPoint destination_last;

  ReachFan reach_terrain;
  ReachFan reach_working;

  RoutePlannerConfig::Polar reach_polar_mode;

  mutable unsigned long count_dij;
  mutable unsigned long count_unique;
  mutable unsigned long count_supressed;

protected:
  RoutePoint astar_goal;
  mutable unsigned long count_airspace;
  mutable unsigned long count_terrain;

public:
  friend class PrintHelper;

  /**
   * Constructor without initialisation of performance model and wind environment.
   * Call update_polar to update the aircraft performance model or wind estimate
   * after initialisation.
   */
  RoutePlanner();

  /**
   * Set terrain database
   * @param terrain RasterMap to be used for terrain intersection tests
   */
  void SetTerrain(const RasterMap *_terrain) {
    terrain = _terrain;
  }

  bool IsTerrainReachEmpty() const {
    return reach_terrain.IsEmpty();
  }

  /**
   * Delete all reach fans.
   */
  void ClearReach();

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
             int h_ceiling = INT_MAX);

  /**
   * Solve reach footprint to terrain
   *
   * @param origin The start of the search (current aircraft location)
   * @param do_solve actually solve or just perform minimal calculations
   *
   * @return True if reach was scanned
   */
  bool SolveReachTerrain(const AGeoPoint &origin, const RoutePlannerConfig &config,
                         int h_ceiling, bool do_solve=true);

  /**
   * Solve reach footprint to working height
   *
   * @param origin The start of the search (current aircraft location)
   * @param do_solve actually solve or just perform minimal calculations
   *
   * @return True if reach was scanned
   */
  bool SolveReachWorking(const AGeoPoint &origin, const RoutePlannerConfig &config,
                         int h_ceiling, bool do_solve=true);

  const FlatProjection &GetTerrainReachProjection() const {
    return reach_terrain.GetProjection();
  }

  /** Visit reach (working or terrain reach) */
  void AcceptInRange(const GeoBounds &bounds,
                     FlatTriangleFanVisitor &visitor,
                     bool working) const;

  /**
   * Retrieve current solution.  If solver failed previously,
   * direct flight from origin to destination is produced.
   */
  const Route &GetSolution() const {
    return solution_route;
  }

  /**
   * Update aircraft performance model used for path planning.
   *
   * @param polar Glide performance model used for route planning
   * @param polar Glide performance model used for reach planning
   * @param wind Wind estimate
   * @param height_min_working Minimum working height (m)
   */
  void UpdatePolar(const GlideSettings &settings,
                   const RoutePlannerConfig &config,
                   const GlidePolar &polar, const GlidePolar &safety_polar,
                   const SpeedVector &wind,
                   const int height_min_working=0);

  /** Reset the optimiser as if never flown and clear temporary buffers. */
  virtual void Reset();

  /**
   * Determine if intersection with terrain occurs in forwards direction from
   * origin to destination, with cruise-climb and glide segments.
   *
   * @param origin Aircraft location
   * @param destination Target
   * @param intx First intercept point
   *
   * @return location of intersection, or GeoPoint::Invalid() if none
   * was found
   */
  gcc_pure
  GeoPoint Intersection(const AGeoPoint &origin,
                        const AGeoPoint &destination) const;

  /**
   * Find arrival height at destination.
   *
   * Requires solve_reach() to have been called for positive results.
   *
   * @param dest Destination location
   * @param arrival_height_reach height at arrival (terrain reach) or -1 if out of reach
   * @param arrival_height_direct height at arrival (pure glide reach) or -1 if out of reach
   *
   * @return true if check was successful
   */
  bool FindPositiveArrival(const AGeoPoint &dest,
                           ReachResult &result_r) const {
    return reach_terrain.FindPositiveArrival(dest, rpolars_reach, result_r);
  }

  int GetTerrainBase() const {
    return reach_terrain.GetTerrainBase();
  }

protected:
  /**
   * Test whether a solution is required or the solution is trivial
   * (too short, etc.)
   *
   * @return True if solution is trivial
   */
  virtual bool IsTrivial() const {
    return !dirty;
  }

  /**
   * Add a link to candidates for search
   *
   * @param e Link to add to candidates
   */
  void AddCandidate(const RouteLink &e);

  /**
   * Add a link to candidates for search
   * (where detailed link data is not provided)
   *
   * @param e Link to add to candidates
   */
  void AddCandidate(const RouteLinkBase &e);

  /**
   * Attempt to add a candidate link skipping the previous
   * point to this point.
   *
   * @param p Point to find shortcut to
   */
  void AddShortcut(const RoutePoint &p);

  /**
   * If this link is truly achievable, add or update
   * the link in the A* search algorithm.
   *
   * @param e Link to clear
   *
   * @return True if link was achievable
   */
  bool LinkCleared(const RouteLink &e);

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
  bool IsSetUnique(const RouteLinkBase &e);

  /**
   * Given a desired path e, and a clearance point, generate candidates directly
   * to the clearance point and to either side.  This method excludes points
   * inside the convex hull of points already visited, so it eliminates
   * backtracking and redundancy.
   *
   * @param inx Clearance point from last check
   * @param e Link that was attempted
   */
  void AddNearbyTerrain(const RoutePoint &inx, const RouteLink& e);

  /**
   * Check whether a desired link may be flown without intersecting with
   * terrain.  If it does, find also the first location that is clear to
   * the destination.
   *
   * @param e Link to attempt
   * @param inp Output clearance point if intersection occurs
   *
   * @return True if path is clear
   */
  bool CheckClearanceTerrain(const RouteLink &e, RoutePoint& inp) const;

private:
  /**
   * Check a second category of obstacle clearance.  This allows compound
   * obstacle categories by subclasses.
   *
   * @param e Link to attempt
   *
   * @return True if path is clear
   */
  virtual bool CheckSecondary(const RouteLink &e) {
    return true;
  }

  /**
   * Check whether a desired link may be flown without intersecting with
   * any obstacle.  If it does, find also the first location that is clear to
   * the destination.
   *
   * @param e Link to attempt
   * @param inp Output clearance point if intersection occurs
   *
   * @return True if path is clear
   */

  virtual bool CheckClearance(const RouteLink &e, RoutePoint& inx) const = 0;

  /**
   * Given a desired path e, and a clearance point, generate candidates directly
   * to the clearance point and elsewhere in attempt to avoid the obstacle.
   *
   * @param inx Clearance point from last check
   * @param e Link that was attempted
   */
  virtual void AddNearby(const RouteLink &e) = 0;

  /**
   * Hook to allow subclasses to update internal data at start of solve() call
   *
   * @param origin origin of search
   * @param destination destination of search
   */
  virtual void OnSolve(const AGeoPoint& origin, const AGeoPoint& destination);

  /**
   * Generate a candidate to left or right of the clearance point, unless:
   * - it is too short
   * - the candidate is more than 90 degrees from the target
   * - the candidate is inside the search area so far
   *
   * @param p Clearance point
   * @param c_link Attempted path
   * @param sign +1 for left, -1 for right
   */
  void AddNearbyTerrainSweep(const RoutePoint& p, const RouteLink &c_link, const int sign);

  /**
   * For a link known to not clear obstacles, generate whatever candidate edges
   * are required to attempt to avoid the obstacles or at least to continue searching.
   *
   * @param e Link to check
   */
  void AddEdges(const RouteLink &e);

  /**
   * Test whether a candidate destination is inside the area already searched
   * or if it would extend the search area.
   *
   * @param p Candidate to test
   *
   * @return True if the candidate is outside the hull
   */
  bool IsHullExtended(const RoutePoint& p);

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
                        Route& this_route) const;
};

#endif
