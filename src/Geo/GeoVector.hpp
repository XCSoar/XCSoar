// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"

#include <type_traits>

struct GeoPoint;

/**
 * A constant bearing vector in lat/lon coordinates.  
 * Should later be extended to handle
 * separately constant bearing and minimum-distance paths. 
 *
 */
struct GeoVector {
  /** Distance in meters */
  double distance;

  /** Bearing (true north) */
  Angle bearing;

  /** Empty non-initializing constructor */
  GeoVector() = default;

  /** Constructor given supplied distance/bearing */
  constexpr
  GeoVector(double _distance, Angle _bearing)
    :distance(_distance), bearing(_bearing) {}

  /**
   * Constructor given start and end location.  
   * Computes Distance/Bearing internally. 
   */
  GeoVector(const GeoPoint &source, const GeoPoint &target);

  /**
   * Create the zero vector: zero distance, undefined bearing.
   */
  constexpr static GeoVector Zero() {
    return GeoVector(0, Angle::Zero());
  }

  /**
   * Create an invalid instance.
   */
  constexpr
  static GeoVector Invalid() {
    return GeoVector(-1, Angle::Zero());
  }

  /**
   * Returns the end point of the geovector projected from the start point.  
   * Assumes constant bearing. 
   */
  [[gnu::pure]]
  GeoPoint EndPoint(const GeoPoint &source) const;

  /**
   * Returns the end point of the geovector projected from the start point.  
   * Assumes constand Bearing. 
   *
   * @param source start of vector
   * @return location of end point
   */
  GeoPoint MidPoint(const GeoPoint &source) const;

  constexpr
  inline bool IsValid() const {
    return distance >= 0;
  }

  void SetInvalid() {
    distance = -1;
  }
};

static_assert(std::is_trivial<GeoVector>::value, "type is not trivial");
