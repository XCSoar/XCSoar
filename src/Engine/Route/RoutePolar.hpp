/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#ifndef ROUTEPOLAR_HPP
#define ROUTEPOLAR_HPP

#include <limits.h>
#include "Task/TaskBehaviour.hpp"
#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Navigation/SearchPoint.hpp"
#include <utility>

class GlidePolar;
struct GlideResult;
class TaskProjection;
class RasterMap;
struct SpeedVector;

typedef AFlatGeoPoint RoutePoint;

/**
 * Class used for primitive 3d navigation links.
 *
 * For route planning, these routes are defined in reverse time order,
 * that is, the first link is the destination (later in time), second link
 * is the origin (earlier in time).
 */
struct RouteLinkBase {
  RouteLinkBase (const RoutePoint& _dest,
                 const RoutePoint& _origin): first(_dest), second(_origin) {}
  RoutePoint first;                                         /**< Destination location */
  RoutePoint second;                                        /**< Origin location */

  /**
   * Equality comparison operator
   *
   * @param o object to compare to
   *
   * @return true if origins and destinations are equal
   */
  gcc_pure
  bool operator==(const RouteLinkBase &o) const {
    return (first == o.first) &&
      (second == o.second);
  }

  /**
   * Ordering operator, used for set ordering.  Uses lexicographic comparison.
   *
   * @param o object to compare to
   *
   * @return true if lexicographically smaller
   */
  gcc_pure
  bool operator< (const RouteLinkBase &o) const {
    if (first.Longitude != o.first.Longitude) return first.Longitude < o.first.Longitude;
    if (first.Latitude != o.first.Latitude) return first.Latitude < o.first.Latitude;
    if (first.altitude != o.first.altitude) return first.altitude < o.first.altitude;
    if (second.Longitude != o.second.Longitude) return second.Longitude < o.second.Longitude;
    if (second.Latitude != o.second.Latitude) return second.Latitude < o.second.Latitude;
    if (second.altitude != o.second.altitude) return second.altitude < o.second.altitude;
    return false;
   }

  /**
   * Return 2d distance of this link
   * @return distance in FlatGeoPoint units
   */
  gcc_pure
  unsigned distance() const {
    return first.distance_to(second);
  }

  /**
   * Test whether this link is too short to be considered
   * for path planning (based on manhattan distance).
   *
   * @return true if this link is short
   */
  gcc_pure
  bool is_short() const;

  /**
   * Calculate the dot product of this link with another.
   * Can be used to test projection of one link in direction of
   * another.
   *
   * @param o second object in dot product
   *
   * @return dot product of this object with second object
   */
  gcc_pure
  int dot(const RouteLinkBase& o) const {
    return (o.second-o.first).dot(second-first);
  }

  /**
   * Calculate the cross product of this link with another.
   * Can be used to test orthogonality of two links.
   *
   * @param o second object in cross product
   *
   * @return cross product of this object with second object
   */
  gcc_pure
  int cross(const RouteLinkBase& o) const {
    return (o.second-o.first).cross(second-first);
  }

};

/**
 * Extension of RouteLinkBase to store additional data
 * on actual distance, reciprocal of distance, and direction indices
 * for fast lookup of performance via RoutePolars.
 */
struct RouteLink: public RouteLinkBase {
public:
  RouteLink (const RouteLinkBase& link,
             const TaskProjection& proj);
  RouteLink (const RoutePoint& _first,
             const RoutePoint& _second,
             const TaskProjection& proj);
  fixed d;                                                  /**< Distance (m) */
  fixed inv_d;                                              /**< Reciprocal of
                                                             * distance (1/m) */
  unsigned polar_index;                                     /**< Direction index
                                                             * to be used for
                                                             * RoutePolar lookups*/

  /**
   * Generate RouteLink projected flat such that the destination altitude equals
   * the start altitude.  The start altitude is unaffected.
   *
   * @return link equivalent to this link flattened
   */
  RouteLink flat() const;
private:
  void calc_speedups(const TaskProjection& proj);
};

#define ROUTEPOLAR_Q0 (6)
#define ROUTEPOLAR_Q1 (2*ROUTEPOLAR_Q0-1)
#define ROUTEPOLAR_Q2 (4*ROUTEPOLAR_Q0)
#define ROUTEPOLAR_Q3 (8*ROUTEPOLAR_Q0)
#define ROUTEPOLAR_POINTS (ROUTEPOLAR_Q3+1)

/**
 * Class to store fast lookup aircraft performance (glide slope and speed) as a
 * function of direction, for a particular GlidePolar and Wind condition.
 * Enables fast performance simulation without using GlidePolar calls.
 *
 */
class RoutePolar {

  /**
   * Structure to hold aircraft performance for a single direction
   */
  struct RoutePolarPoint {
    RoutePolarPoint():slowness(fixed_zero), gradient(fixed_zero), inv_gradient(fixed_zero), valid(false) {};
    RoutePolarPoint(const fixed &_slowness, const fixed &_gradient):slowness(_slowness),gradient(_gradient),valid(true)
      {
        if (positive(gradient))
          inv_gradient = fixed_one/gradient;
        else
          inv_gradient = fixed_zero;
      };
    fixed slowness; /**< Inverse speed (s/m) */
    fixed gradient; /**< Glide slope gradient (m loss / m travelled) */
    fixed inv_gradient; /**< Reciprocal gradient (m travelled / m loss) */
    bool valid; /**< Whether this solution is valid (non-zero speed) */
  };

public:
  /**
   * Copy constructor
   */
  RoutePolar(const RoutePolar& from);

  /**
   * Dummy constructor, does nothing!
   */
  RoutePolar() {};

  /**
   * Populate internal structure with performance data.
   * To be called when the glide polar settings or wind changes.
   *
   * @param polar GlidePolar used to obtain performance data
   * @param wind Wind condition
   * @param glide Whether pure glide or cruise-climb is enforced
   */
  void initialise(const GlidePolar& polar,
                  const SpeedVector& wind,
                  const bool glide);

  /**
   * Retrieve data corresponding to a particular (backwards-time) direction.
   *
   * @param index Index of direction (no range checking is performed!)
   *
   * @return RoutePolarPoint data corresponding to this direction index
   */
  const RoutePolarPoint& get_point(const int index) const {
    return points[index];
  }

  /**
   * Calculate direction index from x,y offsets.  This is equivalent
   * to a fast, inaccurate atan2 returning an index.
   *
   * @param dx X distance units
   * @param dy Y distance units
   *
   * @return Direction index
   */
  static int dxdy_to_index(const int dx, const int dy);

  /**
   * Calculate direction index from normalised x,y offsets.  This is equivalent
   * to a fast, inaccurate atan2 returning an index.
   *
   * @param dx X distance units
   * @param dy Y distance units
   *
   * @return Direction index
   */
  static int dxdy_to_index_norm(const int dx, const int dy);

  /**
   * Calculate distances normalised to 128 corresponding to direction index
   *
   * @param index Direction index
   * @param dx X distance units
   * @param dy Y distance units
   */
  static void index_to_dxdy(const int index, int& dx, int& dy);

private:
  GlideResult solve_task(const GlidePolar& polar, const SpeedVector& wind,
                         const Angle theta, const bool glide) const;

  RoutePolarPoint points[ROUTEPOLAR_POINTS];

};


/**
 * Class to contain separate fast-lookup aircraft performance polars
 * for pure glide and cruise-climb modes, with front-end functions to
 * calculate path times which may involve switching between the modes.
 */
class RoutePolars {
public:
  RoutePolars(const GlidePolar& polar, const SpeedVector& wind);

  /**
   * Re-initialise performance tables when polar or wind changes
   *
   * @param polar Polar used for performance
   * @param wind Wind condition
   */
  void initialise(const GlidePolar& polar, const SpeedVector& wind);

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
  unsigned calc_time(const RouteLink& link) const;

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
  bool check_clearance(const RouteLink &e, const RasterMap* map,
                       const TaskProjection &proj,
                       RoutePoint& inp) const;

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
  RouteLink neighbour_link(const RoutePoint &start,
                           const RoutePoint &end,
                           const TaskProjection &proj,
                           const int sign) const;

  short cruise_altitude; /**< Altitude (m) above which climbs become slow */
  short climb_ceiling; /**< Altitude (m) above which the aircraft cannot climb */

  bool can_climb() const; /**< Whether climbs are possible/allowed */

  /**
   * Calculate the glide height that would be used up in
   * a pure glide travelling the distance and direction of this link.
   *
   * @param link Link to evaluate
   *
   * @return Height of glide (m)
   */
  short calc_vheight(const RouteLink& link) const;

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
  RouteLink generate_intermediate (const RoutePoint& _dest,
                                   const RoutePoint& _origin,
                                   const TaskProjection& proj) const;
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
  bool achievable(const RouteLink& link, const bool check_ceiling=false) const;

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
  void set_config(const RoutePlannerConfig& _config,
                  const short _cruise_alt = SHRT_MAX,
                  const short _ceiling_alt = SHRT_MAX);

  /**
   * Check whether the configuration requires intersection tests with airspace.
   *
   * @return True if airspace tests are required
   */
  bool airspace_enabled() const {
    return config.airspace_enabled();
  }

  /**
   * Check whether the configuration requires intersection tests with terrain.
   *
   * @return True if terrain tests are required
   */
  bool terrain_enabled() const {
    return config.terrain_enabled();
  }

  /**
   * Check whether turns around obstacles are allowed in reach calculations
   *
   * @return True if allow turns, otherwise straight
   */
  bool turning_reach() const {
    return config.turning_reach;
  }

  /**
   * round up just below nearest 8 second block in a quick way
   * this is an attempt to stabilise solutions
   */
  static unsigned round_time(const unsigned val);

  /**
   * Determine if intersection with terrain occurs in forwards direction from
   * origin to destination, with cruise-climb and glide segments.
   *
   * @param origin Aircraft location
   * @param destination Target
   * @param map RasterMap of terrain.
   * @param proj Task projection
   * @param intx First intercept point (output)
   *
   * @return true if terrain intersects
   */
  bool intersection(const AGeoPoint& origin,
                    const AGeoPoint& destination,
                    const RasterMap* map,
                    const TaskProjection& proj,
                    GeoPoint& intx) const;

  /**
   * Calculate height of arrival at destination starting from origin
   */
  short calc_glide_arrival(const AFlatGeoPoint& origin,
                           const FlatGeoPoint& dest,
                           const TaskProjection& proj) const;

  short safety_height() const {
    return (short)config.safety_height_terrain;
  }

  FlatGeoPoint reach_intercept(const int index,
                               const AGeoPoint& p,
                               const RasterMap* map,
                               const TaskProjection& proj) const;

private:

  RoutePolar polar_glide;
  RoutePolar polar_cruise;
  fixed inv_M; /**< Reciprocal of MacCready value (s/m) */

  RoutePlannerConfig config;

  GeoPoint msl_intercept(const int index, const AGeoPoint& p, const TaskProjection& proj) const;
};

#endif
