// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RoutePlanner.hpp"

class ReachFan;

/**
 * Specialization of #RoutePlanner which implements terrain avoidance.
 *
 * Since this class calls RasterMap functions repeatedly, rather than
 * acquiring and releasing locks each time, we assume the hookup to
 * the main program (RoutePlannerGlue) is responsible for locking the
 * RasterMap on solve() calls.
 */
class TerrainRoute: public RoutePlanner
{
  /** Terrain raster */
  const RasterMap *terrain = nullptr;

  /** Aircraft performance model for reach to terrain */
  RoutePolars rpolars_reach;
  /** Aircraft performance model for reach to working floor */
  RoutePolars rpolars_reach_working;

  mutable RoutePoint m_inx_terrain;

public:
  friend class PrintHelper;

  /**
   * Set terrain database
   * @param terrain RasterMap to be used for terrain intersection tests
   */
  void SetTerrain(const RasterMap *_terrain) noexcept {
    terrain = _terrain;
  }

  const auto &GetReachPolar() const noexcept {
    return rpolars_reach;
  }

  void UpdatePolar(const GlideSettings &settings,
                   const RoutePlannerConfig &config,
                   const GlidePolar &task_polar,
                   const GlidePolar &safety_polar,
                   const SpeedVector &wind,
                   int height_min_working=0) noexcept;

  /**
   * Solve reach footprint to terrain or working height.
   *
   * @param origin The start of the search (current aircraft location)
   * @param do_solve actually solve or just perform minimal calculations
   */
  [[gnu::pure]]
  ReachFan SolveReach(const AGeoPoint &origin,
                      const RoutePlannerConfig &config,
                      int h_ceiling, bool do_solve,
                      bool working) noexcept;

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
  [[gnu::pure]]
  GeoPoint Intersection(const AGeoPoint &origin,
                        const AGeoPoint &destination) const noexcept;

protected:
  bool IsClear(const RouteLink &e) const noexcept override;
  void AddNearby(const RouteLink &e) noexcept override;

  /**
   * Check a second category of obstacle clearance.  This allows compound
   * obstacle categories by subclasses.
   *
   * @param e Link to attempt
   *
   * @return True if path is clear
   */
  virtual bool CheckSecondary([[maybe_unused]] const RouteLink &e) noexcept {
    return true;
  }

private:
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
  void AddNearbyTerrainSweep(const RoutePoint& p, const RouteLink &c_link,
                             int sign) noexcept;

  /**
   * Given a desired path e, and a clearance point, generate candidates directly
   * to the clearance point and to either side.  This method excludes points
   * inside the convex hull of points already visited, so it eliminates
   * backtracking and redundancy.
   *
   * @param inx Clearance point from last check
   * @param e Link that was attempted
   */
  void AddNearbyTerrain(const RoutePoint &inx, const RouteLink &e) noexcept;
};
