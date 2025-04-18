// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SectorZone.hpp"

class AnnularSectorZone:
  public SectorZone
{
  double inner_radius;

protected:
  constexpr AnnularSectorZone(Shape _shape, bool _can_start_through_top,
                              const GeoPoint &loc,
                              const double _radiusOuter = 10000.0,
                              const Angle _startRadial = Angle::Zero(),
                              const Angle _endRadial = Angle::FullCircle(),
                              const double _inner_radius = 0.0) noexcept
    :SectorZone(_shape, _can_start_through_top, true, loc, _radiusOuter,
                _startRadial, _endRadial),
     inner_radius(_inner_radius) {}

public:
  constexpr AnnularSectorZone(const AnnularSectorZone &other,
                              const GeoPoint &reference) noexcept
    :SectorZone((const SectorZone &)other, reference),
     inner_radius(other.inner_radius) {}

  /**
   * Constructor
   *
   * @param loc Location of tip of sector
   * @param _radius Radius of sector (m)
   * @param _startRadial Start radial (degrees), most counter-clockwise
   * @param _endRadial End radial (degrees), most clockwise
   *
   * @return Initialised object
   */
  AnnularSectorZone(const GeoPoint &loc,
                    const double _radiusOuter=10000.0,
                    const Angle _startRadial = Angle::Zero(),
                    const Angle _endRadial = Angle::FullCircle(),
                    const double _inner_radius = 0.0) noexcept
    :SectorZone(Shape::ANNULAR_SECTOR, true, true, loc,
                _radiusOuter, _startRadial, _endRadial),
     inner_radius(_inner_radius)
  {
    UpdateSector();
  }

  /**
   * Set inner radius of annulus
   *
   * @param x Radius (m) of inner boundary
   */
  void SetInnerRadius(const double new_radius) noexcept {
    inner_radius = new_radius;
    if (new_radius > GetRadius())
      CylinderZone::SetRadius(new_radius);
    UpdateSector();
  }

  /**
   * Get inner radius property value
   *
   * @return Radius (m) of inner boundary
   */
  constexpr double GetInnerRadius() const noexcept {
    return inner_radius;
  }

  /* virtual methods from class ObservationZone */
  bool IsInSector(const GeoPoint &location) const noexcept override;
  OZBoundary GetBoundary() const noexcept override;

  /* virtual methods from class ObservationZonePoint */
  bool Equals(const ObservationZonePoint &other) const noexcept override;

  std::unique_ptr<ObservationZonePoint> Clone(const GeoPoint &_reference) const noexcept override {
    return std::make_unique<AnnularSectorZone>(*this, _reference);
  }

  /* virtual methods from class CylinderZone */
  void SetRadius(double new_radius) noexcept override {
    CylinderZone::SetRadius(new_radius);
    if (new_radius < inner_radius)
      inner_radius = new_radius;
    UpdateSector();
  }
};
