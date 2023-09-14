// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Type.hpp"
#include "Geo/GeoPoint.hpp"

/**
 * Base class for all task points 
 */
class TaskPoint
{
  TaskPointType type;

  GeoPoint location;

public:
  /**
   * Constructor.  Location and elevation of waypoint is used
   * as the task point's reference values; a copy of the waypoint
   * is also stored to facilitate user-feedback.
   *
   * @param tb Task Behaviour defining options (esp safety heights)
   *
   * @return Initialised object
   */
  constexpr TaskPoint(TaskPointType _type, const GeoPoint &_location) noexcept
    :type(_type), location(_location) {}

  constexpr TaskPointType GetType() const noexcept {
    return type;
  }

  constexpr bool IsIntermediatePoint() const noexcept {
    return type == TaskPointType::AST || type == TaskPointType::AAT;
  }

  /**
   * Retrieve location to be used for remaining task
   * (for a pure TaskPoint, this is the reference location)
   *
   * @return Location
   */
  [[gnu::pure]]
  virtual const GeoPoint &GetLocationRemaining() const noexcept {
    return location;
  }

  /**
   * Calculate vector from aircraft to destination
   *
   * @return Vector for task leg
   */
  [[gnu::pure]]
  virtual GeoVector GetVectorRemaining(const GeoPoint &reference) const noexcept = 0;

  /**
    * Calculate vector of next leg, if there is one
    *
    * @return Vector for task leg or GeoVector::Invalid() if there is no next leg
    */
  [[gnu::pure]]
  virtual GeoVector GetNextLegVector() const noexcept;

  /**
   * Capability of this TaskPoint to have adjustable range/target
   *
   * @return True if task point has a target (can have range set)
   */
  [[gnu::pure]]
  bool HasTarget() const noexcept {
    return type == TaskPointType::AAT;
  }

  /**
   * Retrieve elevation of taskpoint, taking into account
   * rules and safety margins.
   *
   * @return Minimum allowable elevation of task point
   */
  [[gnu::pure]]
  virtual double GetElevation() const noexcept = 0;

  /**
   * distance from this to the reference
   */
  double Distance(const GeoPoint &ref) const noexcept {
    return location.Distance(ref);
  }

  /**
   * The actual location
   */
  const GeoPoint &GetLocation() const noexcept {
    return location;
  }
};
