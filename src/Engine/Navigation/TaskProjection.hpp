/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef TASKPROJECTION_H
#define TASKPROJECTION_H

#include "GeoPoint.hpp"
#include "Compiler.h"

struct FlatPoint;
struct FlatGeoPoint;
class FlatBoundingBox;
struct GeoBounds;

/**
 * Class for performing Lambert Conformal Conic projections from
 * ellipsoidal Earth points to and from projected points.  Has
 * converters for projected coordinates in integer and double types.
 *
 * Needs to be initialized with reset() before first use.
 */
class TaskProjection {
  /** Lower left corner found in scan */
  GeoPoint location_min;
  /** Upper right corner found in scan */
  GeoPoint location_max;
  /** Midpoint of boundary, used as projection center point */
  GeoPoint location_mid;
  /** Cosine of the midpoint */
  fixed cos_midloc;
  /**< Reciprocal of cosine of the midpoint */
  fixed r_cos_midloc;

  /**< Approximate scale (m) of grid spacing at center */
  fixed approx_scale;

#ifndef NDEBUG
  /**
   * Was this object initialised by reset()?  Used in assertions.
   */
  bool initialised;
#endif

public:
#ifndef NDEBUG
  TaskProjection():initialised(false) {}
#endif

  /**
   * Reset search bounds
   *
   * @param ref Default value for initial search bounds
   */
  void reset(const GeoPoint &ref);

  /**
   * Check a location against bounds and update them if outside.
   * This does not update the projection itself.
   *
   * @param ref Point to check against bounds
   */
  void scan_location(const GeoPoint &ref);

  /**
   * Update projection.
   *
   * @return True if projection changed
   */
  bool update_fast();

  /**
   * Project a Geodetic point to an integer 2-d representation
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  gcc_pure
  FlatGeoPoint project(const GeoPoint& tp) const;

  /**
   * Projects a GeoBounds to integer 2-d representation bounding box
   *
   * @param bb BB to project
   *
   * @return Projected bounds
   */
  gcc_pure
  FlatBoundingBox project(const GeoBounds& bb) const;

  /**
   * Projects an integer 2-d representation to a Geodetic point
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  gcc_pure
  GeoPoint unproject(const FlatGeoPoint& tp) const;

  /**
   * Projects a integer 2-d representation bounding box to a GeoBounds
   *
   * @param bb BB to project
   *
   * @return Projected bounds
   */
  gcc_pure
  GeoBounds unproject(const FlatBoundingBox& bb) const;

  /**
   * Project a Geodetic point to an floating point 2-d representation
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  gcc_pure
  FlatPoint fproject(const GeoPoint& tp) const;

  /**
   * Projects an integer 2-d representation to a Geodetic point
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  gcc_pure
  GeoPoint funproject(const FlatPoint& tp) const;

  /**
   * Calculates the integer flat earth distance from an actual distance
   * from a Geodetic point.  Note this is approximate.
   *
   * @param tp Point to project
   * @param range Distance (m) from the Geodetic point
   *
   * @return Distance in flat earth projected units
   */
  gcc_pure
  unsigned project_range(const GeoPoint &tp, const fixed range) const;

  /**
   * Calculates the floating point flat earth distance from an actual distance
   * from a Geodetic point.  Note this is approximate.
   *
   * @param tp Point to project
   * @param range Distance (m) from the Geodetic point
   *
   * @return Distance in flat earth projected units
   */
  gcc_pure
  fixed fproject_range(const GeoPoint &tp, const fixed range) const;

  /** 
   * Return center point (projection reference)
   * 
   * @return Center point of task projection
   */
  gcc_pure
  const GeoPoint& get_center() const {
    return location_mid;
  }
  
  /** 
   * Calculate radius of points used in task projection
   * 
   * note: this is an approximation that should only
   * be used for rendering purposes
   *
   * @return Radius (m) from center to edge
   */
  gcc_pure
  fixed ApproxRadius() const; 

  /**
   * Return approximate grid to flat earth scale in meters
   */
  gcc_pure
  fixed get_approx_scale() const {
    return approx_scale;
  }

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const TaskProjection& task_projection);
#endif
};

#endif
