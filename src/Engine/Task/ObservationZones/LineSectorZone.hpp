// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SymmetricSectorZone.hpp"

/**
 * Observation zone represented as a line.
 * Tests for inSector return true if the subject is behind the line
 * (within a semi-circle of diameter equal to the line length).
 * The constraint test ensures transitioning to exit only occurs if the
 * line is crossed (rather than exiting from the back semi-circle).
 */
class LineSectorZone: public SymmetricSectorZone
{
public:
  constexpr LineSectorZone(const LineSectorZone &other,
                           const GeoPoint &reference) noexcept
    :SymmetricSectorZone((const SymmetricSectorZone &)other, reference) {}

  /**
   * Constructor
   *
   * @param loc Location of center point of line
   * @param length Length of line (m)
   *
   * @return Initialised object
   */
  LineSectorZone(const GeoPoint loc, const double length = 1000.0) noexcept
    :SymmetricSectorZone(Shape::LINE, false, false, loc,
                         length / 2, Angle::HalfCircle())
  {
    UpdateSector();
  }

  /**
   * Set length property
   *
   * @param new_length Length (m) of line
   */
  void SetLength(const double new_length) noexcept {
    SetRadius(new_length / 2);
  }
  
  /**
   * Get length property value
   *
   * @return Length (m) of line
   */
  double GetLength() const noexcept {
    return 2 * GetRadius();
  }

  /* virtual methods from class ObservationZone */
  bool TransitionConstraint(const GeoPoint &location,
                            const GeoPoint &last_location) const noexcept override {
    return CylinderZone::IsInSector(location) &&
      CylinderZone::IsInSector(last_location);
  }

  double ScoreAdjustment() const noexcept override;

  /* virtual methods from class ObservationZonePoint */
  std::unique_ptr<ObservationZonePoint> Clone(const GeoPoint &_reference) const noexcept override {
    return std::make_unique<LineSectorZone>(*this, _reference);
  }
};
