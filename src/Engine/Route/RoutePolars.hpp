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

#ifndef ROUTEPOLARS_HPP
#define ROUTEPOLARS_HPP

#include "Config.hpp"
#include "RoutePolar.hpp"
#include "Point.hpp"

#include <limits.h>

class GlidePolar;
struct GlideSettings;
class FlatProjection;
class RasterMap;
struct SpeedVector;
struct GeoPoint;
struct AGeoPoint;
struct FlatGeoPoint;
struct AFlatGeoPoint;
struct RouteLink;

/**
 * Class to contain separate fast-lookup aircraft performance polars
 * for pure glide and cruise-climb modes, with front-end functions to
 * calculate path times which may involve switching between the modes.
 */
class RoutePolars
{
  RoutePolar polar_glide;
  RoutePolar polar_cruise;

  /** Reciprocal of MacCready value (s/m) */
  double inv_mc;

  /** Minimum working height (m) */
  int height_min_working;

  RoutePlannerConfig config;

public:
  /** Altitude (m) above which climbs become slow */
  int cruise_altitude;
  /** Altitude (m) above which the aircraft cannot climb */
  int climb_ceiling;

  /**
   * Re-initialise performance tables when polar or wind changes
   *
   * @param polar Polar used for performance
   * @param wind Wind condition
   */
  void Initialise(const GlideSettings &settings, const GlidePolar& polar,
                  const SpeedVector& wind,
                  const int _height_min_working=0);

  /**
   * Calculate the time required to fly the link.  Returns UINT_MAX
   * if flight is impossible.  Climbs above the cruise altitude
   * are slowed to a fraction of the main polar performance.
   *
   * If the destination is higher than the origin, a single climb
   * is assumed at the origin that does not account for wind drift.
   * Since these propagate to the start of the search, this is a reasonable
   * approximation.
   *
   * @param link Link defining the path to evaluate
   *
   * @return Time (s) elapsed to fly the link
   */
  unsigned CalcTime(const RouteLink& link) const;

  /**
   * Test whether flight through a link intersects with terrain
   * evaluating backwards from the destination to the origin.
   * If it does intersect, returns the first clearance point.
   * If the destination is lower than the glide height from the
   * origin, a jump is made (the actual destination height would
   * be higher than specified).
   *
   * See documentation of RasterTile::FirstIntersecting for more
   * details.
   *
   * @param e Link to evaluate
   * @param map RasterMap of terrain.
   * @param proj Task projection
   * @param inp (output) clearance after intersection point
   *
   * @return True if intersect occurs
   */
  bool CheckClearance(const RouteLink &e, const RasterMap* map,
                      const FlatProjection &proj, RoutePoint &inp) const;

  /**
   * Rotate line from start to end either left or right
   *
   * @param start first node
   * @param end second node, to be rotated around first node
   * @param proj  projection used to generate link
   * @param sign  side to rotate (+1=left, -1=right)
   *
   * @return Rotated link
   */
  RouteLink NeighbourLink(const RoutePoint &start, const RoutePoint &end,
                          const FlatProjection &proj, const int sign) const;

  /** Whether climbs are possible/allowed */
  bool CanClimb() const;

  /**
   * Calculate the glide height that would be used up in
   * a pure glide travelling the distance and direction of this link.
   *
   * @param link Link to evaluate
   *
   * @return Height of glide (m)
   */
  double CalcVHeight(const RouteLink &link) const;

  /**
   * Generate a link from the destination imposing constraints on the origin
   * based on cruise altitude and climb limits.
   *
   * @param _dest Destination point
   * @param _origin Source point suggested
   * @param proj Task projection
   *
   * @return Link
   */
  RouteLink GenerateIntermediate(const RoutePoint& _dest,
                                 const RoutePoint& _origin,
                                 const FlatProjection &proj) const;

  /**
   * Test whether the specified link is achievable given climb potential and
   * climb ceiling limits.  If climbs are not possible, the destination must be
   * reachable by pure glide.
   *
   * @param link Link to test
   * @param check_ceiling Whether to check if link climbs through ceiling
   *
   * @return True if achievable to fly this link
   */
  bool IsAchievable(const RouteLink &link, const bool check_ceiling=false) const;

  /**
   * Set configuration parameters for this performance model.
   * Cruise altitude sets a height above which climbs are penalised (slower than
   * MC climbs).
   * Ceiling altitude sets an absolute limit on maximum altitude if use_ceiling
   * is true in config.
   *
   * @param _config Configuration to set
   * @param _cruise_alt Cruise altitude (m)
   * @param _ceiling_alt Ceiling altitude (m)
   */
  void SetConfig(const RoutePlannerConfig &_config,
                 int _cruise_alt = INT_MAX,
                 int _ceiling_alt = INT_MAX);

  /**
   * Check whether the configuration requires intersection tests with airspace.
   *
   * @return True if airspace tests are required
   */
  bool IsAirspaceEnabled() const {
    return config.IsAirspaceEnabled();
  }

  /**
   * Check whether the configuration requires intersection tests with terrain.
   *
   * @return True if terrain tests are required
   */
  bool IsTerrainEnabled() const {
    return config.IsTerrainEnabled();
  }

  /**
   * Check whether turns around obstacles are allowed in reach calculations
   *
   * @return True if allow turns, otherwise straight
   */
  bool IsTurningReachEnabled() const {
    return config.IsTurningReachEnabled();
  }

  /**
   * round up just below nearest 8 second block in a quick way
   * this is an attempt to stabilise solutions
   */
  static unsigned RoundTime(const unsigned val);

  /**
   * Determine if intersection with terrain occurs in forwards direction from
   * origin to destination, with cruise-climb and glide segments.
   *
   * @param origin Aircraft location
   * @param destination Target
   * @param map RasterMap of terrain.
   * @param proj Task projection
   *
   * @return location of intersection, or GeoPoint::Invalid() if none
   * was found
   */
  gcc_pure
  GeoPoint Intersection(const AGeoPoint &origin, const AGeoPoint &destination,
                        const RasterMap *map,
                        const FlatProjection &proj) const;

  /**
   * Calculate height of arrival at destination starting from origin
   */
  int CalcGlideArrival(const AFlatGeoPoint& origin,
                       const FlatGeoPoint& dest,
                       const FlatProjection &proj) const;

  int GetSafetyHeight() const {
    return config.safety_height_terrain;
  }

  int GetFloor() const {
    return height_min_working;
  }

  gcc_pure
  FlatGeoPoint ReachIntercept(int index, const AFlatGeoPoint &flat_origin,
                              const GeoPoint &origin,
                              const RasterMap* map,
                              const FlatProjection &proj) const;

private:
  gcc_pure
  FlatGeoPoint MSLIntercept(const int index, const FlatGeoPoint &p,
                            double altitude,
                            const FlatProjection &proj) const;
};

#endif
