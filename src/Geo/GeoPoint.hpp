/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_GeoPoint_HPP
#define XCSOAR_GeoPoint_HPP

#include "Math/Angle.hpp"
#include "Rough/RoughAltitude.hpp"
#include "Compiler.h"

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
  GeoPoint() = default;

  /**
   * Constructor (supplied location)
   *
   * @param _Longitude Longitude of point
   * @param _Latitude Latitude of point
   *
   * @return Initialised object
   */
  constexpr
  GeoPoint(const Angle _longitude, const Angle _latitude) :
    longitude(_longitude), latitude(_latitude) {}

  /**
   * Construct an instance at the origin of the coordinate system.
   * This is used to initialise the simulator when there is no better
   * reference (home, map center).  The goal is to bootstrap the
   * simulator when XCSoar is launched for the first time with an
   * empty profile; it is pretty useless for anything else.
   */
  constexpr
  static GeoPoint Zero() {
    return GeoPoint(Angle::Zero(), Angle::Zero());
  }

  /**
   * Construct an instance that is "invalid", i.e. IsValid() will
   * return false.  The return value must not be used in any
   * calculation.  This method may be used to explicitly declare a
   * GeoPoint attribute as "invalid".
   */
  constexpr
  static GeoPoint Invalid() {
    return GeoPoint(Angle::Zero(), Angle::FullCircle());
  }

  /**
   * Set this instance to "invalid", i.e. IsValid() will
   * return false.  The return value must not be used in any
   * calculation.  This method may be used to explicitly declare a
   * GeoPoint attribute as "invalid".
   */
  void SetInvalid() {
    longitude = Angle::Zero();
    latitude = Angle::FullCircle();
  }

  /**
   * Check if this object is "valid".  Returns false when it was
   * constructed by Invalid().  This is not an extensive plausibility
   * check; it is only designed to catch instances created by
   * Invalid().
   */
  constexpr
  bool IsValid() const {
    return latitude <= Angle::HalfCircle();
  }

  /**
   * Check if both longitude and latitude are in the allowed range.
   */
  constexpr bool Check() const {
    return longitude >= -Angle::HalfCircle() &&
      longitude <= Angle::HalfCircle() &&
      latitude >= -Angle::QuarterCircle() &&
      latitude <= Angle::QuarterCircle();
  }

  /**
   * Normalize the values, so this object can be used properly in
   * calculations, without unintended side effects (such as -1 degrees
   * vs 359 degrees).  This modification is in-place.
   */
  GeoPoint &Normalize() {
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
  gcc_pure
  GeoPoint Parametric(const GeoPoint &delta, const fixed t) const;

  /**
   * Find location interpolated from this point towards end
   *
   * @param end Endpoint of interpolation
   * @param t Parametric distance along this to end [0,1]
   *
   * @return Location of point
   */
  gcc_pure
  GeoPoint Interpolate(const GeoPoint &end, const fixed t) const;

  /**
   * Multiply a point by a factor (used for deltas)
   *
   * @param x Factor to magnify
   *
   * @return Modified point
   */
  gcc_pure
  GeoPoint operator* (const fixed x) const {
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
  gcc_pure
  GeoPoint operator+ (const GeoPoint &delta) const {
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
  const GeoPoint& operator+= (const GeoPoint &delta) {
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
  gcc_pure
  GeoPoint operator- (const GeoPoint &delta) const {
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
  gcc_pure
  fixed Distance(const GeoPoint &other) const;

  /**
   * Calculate great circle initial bearing from this to the other
   *
   * @param other Other location
   *
   * @return Bearing (deg)
   */
  gcc_pure
  Angle Bearing(const GeoPoint &other) const;

  /**
   * Calculate great circle distance and initial bearing from this to the other
   */
  gcc_pure
  GeoVector DistanceBearing(const GeoPoint &other) const;

  /**
   * Like Distance(), but use a simplified faster formula that may be
   * less accurate.
   */
  gcc_pure
  fixed DistanceS(const GeoPoint &other) const;

  /**
   * Find distance along a great-circle path that this point
   * is projected to
   *
   * @param from Start location
   * @param to End location
   *
   * @return Distance (m) along from-to line
   */
  gcc_pure
  fixed ProjectedDistance(const GeoPoint &from, const GeoPoint &to) const;

  /**
   * Find point a set distance along a great-circle path towards
   * a destination
   *
   * @param destination End location
   * @param distance distance (m)
   *
   * @return Location of point
   */
  gcc_pure
  GeoPoint IntermediatePoint(const GeoPoint &destination,
                             const fixed distance) const;

  /**
   * Find the nearest great-circle middle point between this point and
   * the specified one.
   */
  gcc_pure
  GeoPoint Middle(const GeoPoint &other) const;

  /**
   * Test whether two points are co-located
   *
   * @param other Point to compare
   *
   * @return True if coincident
   */
  constexpr
  bool Equals(const GeoPoint other) const {
    return longitude == other.longitude && latitude == other.latitude;
  }

  /**
   * Test whether two points are co-located
   *
   * @param other Point to compare
   *
   * @return True if coincident
   */
  constexpr
  bool operator== (const GeoPoint other) const {
    return Equals(other);
  }

  /**
   * Test whether two points are not co-located
   *
   * @param other Point to compare
   *
   * @return True if coincident
   */
  constexpr
  bool operator !=(const GeoPoint &other) const {
    return !Equals(other);
  }
  
  /**
   * Rank two points according to longitude, then latitude
   *
   * @param other Point to compare to
   *
   * @return True if this point is further left (or if equal, lower) than the other
   */
  gcc_pure
  bool Sort(const GeoPoint &other) const;
};

static_assert(std::is_trivial<GeoPoint>::value, "type is not trivial");

/**
 * Extension of GeoPoint for altitude (3d location in spherical space)
 */
struct AGeoPoint: public GeoPoint {
  /**< Nav reference altitude (m) */
  RoughAltitude altitude;

  AGeoPoint() = default;

  constexpr
  AGeoPoint(const GeoPoint p, const RoughAltitude alt)
    :GeoPoint(p),altitude(alt) {};
};

static_assert(std::is_trivial<AGeoPoint>::value, "type is not trivial");

#endif
