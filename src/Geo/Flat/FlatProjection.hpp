// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

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
  [[gnu::pure]]
  FlatGeoPoint ProjectInteger(const GeoPoint &tp) const;

  /**
   * Projects a GeoBounds to integer 2-d representation bounding box
   *
   * @param bb BB to project
   *
   * @return Projected bounds
   */
  [[gnu::pure]]
  FlatBoundingBox Project(const GeoBounds &bb) const;

  /**
   * Project a square defined by its center and a radius.
   */
  [[gnu::pure]]
  FlatBoundingBox ProjectSquare(const GeoPoint center, double radius) const;

  /**
   * Projects an integer 2-d representation to a Geodetic point
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  [[gnu::pure]]
  GeoPoint Unproject(const FlatGeoPoint &tp) const;

  /**
   * Projects a integer 2-d representation bounding box to a GeoBounds
   *
   * @param bb BB to project
   *
   * @return Projected bounds
   */
  [[gnu::pure]]
  GeoBounds Unproject(const FlatBoundingBox &bb) const;

  /**
   * Project a Geodetic point to an floating point 2-d representation
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  [[gnu::pure]]
  FlatPoint ProjectFloat(const GeoPoint &tp) const;

  /**
   * Projects an integer 2-d representation to a Geodetic point
   *
   * @param tp Point to project
   *
   * @return Projected point
   */
  [[gnu::pure]]
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
  [[gnu::pure]]
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
  [[gnu::pure]]
  double ProjectRangeFloat(const GeoPoint &tp, double range) const;

  /** 
   * Return center point (projection reference)
   * 
   * @return Center point of task projection
   */
  [[gnu::pure]]
  const GeoPoint &GetCenter() const {
    return center;
  }
  
  /**
   * Return approximate grid to flat earth scale in meters
   */
  [[gnu::pure]]
  double GetApproximateScale() const {
    return approx_scale;
  }
};
