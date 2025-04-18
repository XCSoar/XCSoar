// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"
#include "Math/Classify.hpp"

#include <type_traits>

struct GeoVector;

/**
 * Geodetic coordinate expressed as Longitude and Latitude angles.
 */
struct GeoPoint {
  Angle longitude;
  Angle latitude;

  /**
   * Non-initialising constructor.
   */
  GeoPoint() noexcept = default;

  /**
   * Constructor (supplied location)
   *
   * @param _Longitude Longitude of point
   * @param _Latitude Latitude of point
   *
   * @return Initialised object
   */
  constexpr GeoPoint(const Angle _longitude, const Angle _latitude) noexcept
    :longitude(_longitude), latitude(_latitude) {}

  /**
   * Construct an instance at the origin of the coordinate system.
   * This is used to initialise the simulator when there is no better
   * reference (home, map center).  The goal is to bootstrap the
   * simulator when XCSoar is launched for the first time with an
   * empty profile; it is pretty useless for anything else.
   */
  static constexpr GeoPoint Zero() noexcept {
    return GeoPoint(Angle::Zero(), Angle::Zero());
  }

  /**
   * Construct an instance that is "invalid", i.e. IsValid() will
   * return false.  The return value must not be used in any
   * calculation.  This method may be used to explicitly declare a
   * GeoPoint attribute as "invalid".
   */
  static constexpr GeoPoint Invalid() noexcept {
    return GeoPoint(Angle::Zero(), Angle::FullCircle());
  }

  /**
   * Set this instance to "invalid", i.e. IsValid() will
   * return false.  The return value must not be used in any
   * calculation.  This method may be used to explicitly declare a
   * GeoPoint attribute as "invalid".
   */
  constexpr void SetInvalid() noexcept {
    longitude = Angle::Zero();
    latitude = Angle::FullCircle();
  }

  /**
   * Check if this object is "valid".  Returns false when it was
   * constructed by Invalid().  This is not an extensive plausibility
   * check; it is only designed to catch instances created by
   * Invalid().
   */
  constexpr bool IsValid() const noexcept {
    return latitude <= Angle::HalfCircle();
  }

  /**
   * Check if both longitude and latitude are in the allowed range.
   */
  constexpr bool Check() const noexcept {
    return IsFinite(longitude.Native()) &&
      longitude >= -Angle::HalfCircle() &&
      longitude <= Angle::HalfCircle() &&
      IsFinite(latitude.Native()) &&
      latitude >= -Angle::QuarterCircle() &&
      latitude <= Angle::QuarterCircle();
  }

  /**
   * Normalize the values, so this object can be used properly in
   * calculations, without unintended side effects (such as -1 degrees
   * vs 359 degrees).  This modification is in-place.
   */
  GeoPoint &Normalize() noexcept {
    longitude = longitude.AsDelta();

    if (latitude < -Angle::QuarterCircle())
      latitude = -Angle::QuarterCircle();
    else if (latitude > Angle::QuarterCircle())
      latitude = Angle::QuarterCircle();

    return *this;
  }

  /**
   * Find location a parametric distance along a vector from this point
   *
   * @param delta Vector to feed in
   * @param t Parametric distance along delta to add [0,1]
   *
   * @return Location of point
   */
  [[gnu::pure]]
  GeoPoint Parametric(const GeoPoint &delta, double t) const noexcept;

  /**
   * Find location interpolated from this point towards end
   *
   * @param end Endpoint of interpolation
   * @param t Parametric distance along this to end [0,1]
   *
   * @return Location of point
   */
  [[gnu::pure]]
  GeoPoint Interpolate(const GeoPoint &end, double t) const noexcept;

  /**
   * Multiply a point by a factor (used for deltas)
   *
   * @param x Factor to magnify
   *
   * @return Modified point
   */
  [[gnu::pure]]
  constexpr GeoPoint operator*(const double x) const noexcept {
    GeoPoint res = *this;
    res.longitude *= x;
    res.latitude *= x;
    return res;
  };

  /**
   * Add a delta to a point
   *
   * @param delta Delta to add
   *
   * @return Modified point
   */
  [[gnu::pure]]
  constexpr GeoPoint operator+(const GeoPoint &delta) const noexcept {
    GeoPoint res = *this;
    res.longitude += delta.longitude;
    res.latitude += delta.latitude;
    return res;
  };

  /**
   * Add a delta to a point
   *
   * @param delta Delta to add
   *
   * @return Modified point
   */
  constexpr const GeoPoint &operator+=(const GeoPoint &delta) noexcept {
    longitude += delta.longitude;
    latitude += delta.latitude;
    return *this;
  };

  /**
   * Subtracts a delta from a point
   *
   * @param delta Delta to subtract
   *
   * @return Modified point
   */
  [[gnu::pure]]
  GeoPoint operator-(const GeoPoint &delta) const noexcept {
    GeoPoint res = *this;
    res.longitude -= delta.longitude;
    res.latitude -= delta.latitude;
    return res.Normalize();
  };

  /**
   * Calculate great circle distance from this to the other
   *
   * @param other Other location
   *
   * @return Distance (m)
   */
  [[gnu::pure]]
  double Distance(const GeoPoint &other) const noexcept;

  /**
   * Calculate great circle initial bearing from this to the other
   *
   * @param other Other location
   *
   * @return Bearing (deg)
   */
  [[gnu::pure]]
  Angle Bearing(const GeoPoint &other) const noexcept;

  /**
   * Calculate great circle distance and initial bearing from this to the other
   */
  [[gnu::pure]]
  GeoVector DistanceBearing(const GeoPoint &other) const noexcept;

  /**
   * Like Distance(), but use a simplified faster formula that may be
   * less accurate.
   */
  [[gnu::pure]]
  double DistanceS(const GeoPoint &other) const noexcept;

  /**
   * Like Bearing(), but use a simplified faster formula that may be
   * less accurate.
   */
  [[gnu::pure]]
  Angle BearingS(const GeoPoint &other) const noexcept;

  /**
   * Like DistanceBearing(), but use a simplified faster formula that
   * may be less accurate.
   */
  [[gnu::pure]]
  GeoVector DistanceBearingS(const GeoPoint &other) const noexcept;

  /**
   * Find distance along a great-circle path that this point
   * is projected to
   *
   * @param from Start location
   * @param to End location
   *
   * @return Distance (m) along from-to line
   */
  [[gnu::pure]]
  double ProjectedDistance(const GeoPoint &from,
                           const GeoPoint &to) const noexcept;

  /**
   * Find point a set distance along a great-circle path towards
   * a destination
   *
   * @param destination End location
   * @param distance distance (m)
   *
   * @return Location of point
   */
  [[gnu::pure]]
  GeoPoint IntermediatePoint(const GeoPoint &destination,
                             double distance) const noexcept;

  /**
   * Find the nearest great-circle middle point between this point and
   * the specified one.
   */
  [[gnu::pure]]
  GeoPoint Middle(const GeoPoint &other) const noexcept;

  constexpr bool operator==(const GeoPoint &) const noexcept = default;
};

static_assert(std::is_trivial<GeoPoint>::value, "type is not trivial");

/**
 * Extension of GeoPoint for altitude (3d location in spherical space)
 */
struct AGeoPoint: public GeoPoint {
  /**< Nav reference altitude (m) */
  double altitude;

  AGeoPoint() noexcept = default;

  constexpr AGeoPoint(GeoPoint p, double alt) noexcept
    :GeoPoint(p), altitude(alt) {};
};

static_assert(std::is_trivial<AGeoPoint>::value, "type is not trivial");
