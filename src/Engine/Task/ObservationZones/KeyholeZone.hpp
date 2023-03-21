// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SymmetricSectorZone.hpp"

/**
 * A (default) 90 degree 10km sector centered at the bisector of
 * incoming/outgoing legs, with 500m cylinder
 */
class KeyholeZone final : public SymmetricSectorZone
{
  double inner_radius;

public:
  /**
   * Constructor.
   *
   * @param loc Tip of sector location
   * @param radius Radius (m) of sector
   *
   * @return Initialised object
   */
  constexpr KeyholeZone(Shape _shape, bool _can_start_through_top,
                        bool _arc_boundary,
                        const GeoPoint &loc,
                        const double radius = 10000.0,
                        const Angle angle = Angle::QuarterCircle()) noexcept
    :SymmetricSectorZone(_shape, _can_start_through_top, _arc_boundary,
                         loc, radius, angle),
     inner_radius(500) {}

  constexpr KeyholeZone(const KeyholeZone &other, const GeoPoint &reference) noexcept
    :SymmetricSectorZone((const SymmetricSectorZone &)other, reference),
     inner_radius(other.inner_radius) {}

  static auto CreateCustomKeyholeZone(const GeoPoint &reference,
                                      double radius,
                                      Angle angle) noexcept {
    return std::make_unique<KeyholeZone>(Shape::CUSTOM_KEYHOLE, true, true,
                                         reference, radius, angle);
  }

  /**
   * Create a 90 degree 10km sector centered at the bisector of
   * incoming/outgoing legs, with 500m cylinder, according to DAeC
   * rules.
   */
  static auto CreateDAeCKeyholeZone(const GeoPoint &reference) noexcept {
    return std::make_unique<KeyholeZone>(Shape::DAEC_KEYHOLE,
                                         true, true, reference,
                                         10000,
                                         Angle::QuarterCircle());
  }

  /**
   * Create a 90 degree 20km sector centered at the bisector of
   * incoming/outgoing legs, with 500m cylinder.
   */
  static auto CreateBGAFixedCourseZone(const GeoPoint &reference) noexcept {
    return std::make_unique<KeyholeZone>(Shape::BGAFIXEDCOURSE,
                                         true, true, reference,
                                         20000,
                                         Angle::QuarterCircle());
  }

  /**
   * Create a 180 degree 10km sector centered at the bisector of
   * incoming/outgoing legs, with 500m cylinder
   */
  static auto CreateBGAEnhancedOptionZone(const GeoPoint &reference) noexcept {
    return std::make_unique<KeyholeZone>(Shape::BGAENHANCEDOPTION, true, true, reference,
                                         10000,
                                         Angle::HalfCircle());
  }

  /**
   * Returns the radius of the small cylinder [m].
   */
  constexpr double GetInnerRadius() const noexcept {
    return inner_radius;
  }

  constexpr void SetInnerRadius(double _radius) noexcept {
    inner_radius = _radius;
  }

  /* virtual methods from class ObservationZone */
  bool IsInSector(const GeoPoint &location) const noexcept override;
  OZBoundary GetBoundary() const noexcept override;
  double ScoreAdjustment() const noexcept override;

  /* virtual methods from class ObservationZonePoint */
  std::unique_ptr<ObservationZonePoint> Clone(const GeoPoint &_reference) const noexcept override {
    return std::make_unique<KeyholeZone>(*this, _reference);
  }
};
