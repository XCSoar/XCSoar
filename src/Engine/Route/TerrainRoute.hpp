/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
  std::optional<RoutePoint> CheckClearance(const RouteLink &e) const noexcept override;
  void AddNearby(const RouteLink &e) noexcept override;

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

  /**
   * Check whether a desired link may be flown without intersecting with
   * terrain.  If it does, find also the first location that is clear to
   * the destination.
   *
   * @param e Link to attempt
   *
   * @return std::nullopt if path is clear or clearance point if
   * intersection occurs
   */
  [[gnu::pure]]
  std::optional<RoutePoint> CheckClearanceTerrain(const RouteLink &e) const noexcept;
};
