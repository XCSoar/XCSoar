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

#ifndef XCSOAR_FLAT_PROJECTION_HPP
#define XCSOAR_FLAT_PROJECTION_HPP

#include "Geo/GeoPoint.hpp"
#include "Compiler.h"

struct FlatPoint;
struct FlatGeoPoint;
struct FlatBoundingBox;
class GeoBounds;

/**
 * Class for performing Lambert Conformal Conic projections from
 * ellipsoidal Earth points to and from projected points.  Has
 * converters for projected coordinates in integer and double types.
 *
 * Needs to be initialized with reset() before first use.
 */
class FlatProjection {
  /**
   * The "reference" location, used as projection center point.
   */
  GeoPoint center;

  /**
   * Cosine of the #center location.
   */
  double cos;

  /**
   * Reciprocal of cosine of the #center location.
   */
  double r_cos;

  /**< Approximate scale (m) of grid spacing at center */
  double approx_scale;

public:
  FlatProjection() = default;

  explicit FlatProjection(const GeoPoint &_center) {
    SetCenter(_center);
  }

  bool IsValid() const {
    return center.IsValid();
  }

  /**
   * Marks this projection "invalid", i.e. IsValid() will return false
   * and projection operations are illegal.  This is the opposite of
   * SetCenter().
   */
  void SetInvalid() {
    center = GeoPoint::Invalid();
  }

  /**
   * Sets the new projection center and initialises the projection.
   *
   * After returning, IsValid() returns true.
   */
  void SetCenter(const GeoPoint &_center);

  /**
   * Project a Geodetic point to an integer 2-d representation
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  gcc_pure
  FlatGeoPoint ProjectInteger(const GeoPoint &tp) const;

  /**
   * Projects a GeoBounds to integer 2-d representation bounding box
   *
   * @param bb BB to project
   *
   * @return Projected bounds
   */
  gcc_pure
  FlatBoundingBox Project(const GeoBounds &bb) const;

  /**
   * Project a square defined by its center and a radius.
   */
  gcc_pure
  FlatBoundingBox ProjectSquare(const GeoPoint center, double radius) const;

  /**
   * Projects an integer 2-d representation to a Geodetic point
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  gcc_pure
  GeoPoint Unproject(const FlatGeoPoint &tp) const;

  /**
   * Projects a integer 2-d representation bounding box to a GeoBounds
   *
   * @param bb BB to project
   *
   * @return Projected bounds
   */
  gcc_pure
  GeoBounds Unproject(const FlatBoundingBox &bb) const;

  /**
   * Project a Geodetic point to an floating point 2-d representation
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  gcc_pure
  FlatPoint ProjectFloat(const GeoPoint &tp) const;

  /**
   * Projects an integer 2-d representation to a Geodetic point
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  gcc_pure
  GeoPoint Unproject(const FlatPoint &tp) const;

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
  unsigned ProjectRangeInteger(const GeoPoint &tp, double range) const;

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
  double ProjectRangeFloat(const GeoPoint &tp, double range) const;

  /** 
   * Return center point (projection reference)
   * 
   * @return Center point of task projection
   */
  gcc_pure
  const GeoPoint &GetCenter() const {
    return center;
  }
  
  /**
   * Return approximate grid to flat earth scale in meters
   */
  gcc_pure
  double GetApproximateScale() const {
    return approx_scale;
  }
};

#endif
