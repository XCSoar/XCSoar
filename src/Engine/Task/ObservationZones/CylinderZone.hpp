// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ObservationZonePoint.hpp"

#include <cassert>

/**
 * Observation zone represented as a circular area with specified
 * radius from a center point
 */
class CylinderZone : public ObservationZonePoint
{
  /** radius (m) of OZ */
  double radius;

public:
  static constexpr double MAT_RADIUS = 1609.344;

  constexpr CylinderZone(Shape _shape, bool _can_start_through_top,
                         const GeoPoint &loc,
                         const double _radius = 10000.0) noexcept
    :ObservationZonePoint(_shape, _can_start_through_top, loc),
     radius(_radius) {
    assert(radius > 0);
  }

  constexpr CylinderZone(const CylinderZone &other,
                         const GeoPoint &reference) noexcept
    :ObservationZonePoint((const ObservationZonePoint &)other, reference),
     radius(other.radius) {
    assert(radius > 0);
  }

  /**
   * Constructor.
   *
   * @param loc Location of center
   * @param _radius Radius (m) of cylinder
   *
   * @return Initialised object
   */
  constexpr CylinderZone(const GeoPoint &loc, const double _radius = 10000.0) noexcept
    :ObservationZonePoint(Shape::CYLINDER, true, loc), radius(_radius) {
    assert(radius > 0);
  }

  static auto CreateMatCylinderZone(const GeoPoint &loc) noexcept {
    return std::make_unique<CylinderZone>(Shape::MAT_CYLINDER,
                                          true, loc,
                                          MAT_RADIUS);
  }

  /**
   * Set radius property
   *
   * @param new_radius Radius (m) of cylinder
   */
  virtual void SetRadius(double new_radius) noexcept {
    assert(new_radius > 0);
    radius = new_radius;
  }

  /**
   * Get radius property value
   *
   * @return Radius (m) of cylinder
   */
  double GetRadius() const noexcept {
    return radius;
  }

  /* virtual methods from class ObservationZone */
  bool IsInSector(const GeoPoint &location) const noexcept override {
    return DistanceTo(location) <= radius;
  }

  bool TransitionConstraint([[maybe_unused]] const GeoPoint &location,
                            [[maybe_unused]] const GeoPoint &last_location) const noexcept override {
    return true;
  }

  OZBoundary GetBoundary() const noexcept override;
  double ScoreAdjustment() const noexcept override;

  /* virtual methods from class ObservationZonePoint */
  bool Equals(const ObservationZonePoint &other) const noexcept override;
  GeoPoint GetRandomPointInSector(const double mag) const noexcept override;

  std::unique_ptr<ObservationZonePoint> Clone(const GeoPoint &_reference) const noexcept override {
    return std::make_unique<CylinderZone>(*this, _reference);
  }
};
