// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "CylinderZone.hpp"

/**
 * Sector of finite radius, defined by segment interior
 * to supplied start/end radials
 */
class SectorZone: public CylinderZone
{
  /**
   * Does the boundary include the arc, i.e. is crossing the arc
   * scored?
   */
  const bool arc_boundary;

  /** Location of far end point of start radial */
  GeoPoint sector_start;
  /** Location of far end point of end radial */
  GeoPoint sector_end;

  Angle start_radial;
  Angle end_radial;

protected:
  constexpr SectorZone(Shape _shape, bool _can_start_through_top,
                       bool _arc_boundary,
                       const GeoPoint &loc,
                       const double _radius = 10000.0,
                       const Angle _start_radial = Angle::Zero(),
                       const Angle _end_radial = Angle::FullCircle()) noexcept
    :CylinderZone(_shape, _can_start_through_top, loc, _radius),
     arc_boundary(_arc_boundary),
     start_radial(_start_radial), end_radial(_end_radial) {}

public:
  constexpr SectorZone(const SectorZone &other,
                       const GeoPoint &reference) noexcept
    :CylinderZone((const CylinderZone &)other, reference),
     arc_boundary(other.arc_boundary),
     sector_start(other.sector_start), sector_end(other.sector_end),
     start_radial(other.start_radial), end_radial(other.end_radial) {}

  /**
   * Constructor
   *
   * @param loc Location of tip of sector
   * @param _radius Radius of sector (m)
   * @param _start_radial Start radial (degrees), most counter-clockwise
   * @param _end_radial End radial (degrees), most clockwise
   *
   * @return Initialised object
   */
  SectorZone(const GeoPoint &loc, const double _radius = 10000.0,
             const Angle _start_radial = Angle::Zero(),
             const Angle _end_radial = Angle::FullCircle()) noexcept
    :CylinderZone(Shape::SECTOR, true, loc, _radius),
     arc_boundary(true),
     start_radial(_start_radial), end_radial(_end_radial)
  {
    UpdateSector();
  }

  /**
   * Set start angle (most counter-clockwise) of sector
   *
   * @param x Angle (deg) of radial
   */
  void SetStartRadial(const Angle x) noexcept;

  /**
   * Set end angle (most clockwise) of sector
   *
   * @param x Angle (deg) of radial
   */
  void SetEndRadial(const Angle x) noexcept;

  /**
   * Get start radial property value
   *
   * @return Angle (deg) of radial
   */
  constexpr Angle GetStartRadial() const noexcept {
    return start_radial;
  }

  /**
   * Get end radial property value
   *
   * @return Angle (deg) of radial
   */
  constexpr Angle GetEndRadial() const noexcept {
    return end_radial;
  }

  /** 
   * Retrieve start radial endpoint
   * 
   * @return Location of extreme point on start radial
   */
  constexpr const GeoPoint &GetSectorStart() const noexcept {
    return sector_start;
  }

  /** 
   * Retrieve end radial endpoint
   * 
   * @return Location of extreme point on end radial
   */
  constexpr const GeoPoint &GetSectorEnd() const noexcept {
    return sector_end;
  }

protected:
  /**
   * Updates sector parameters; call this if geometry changes to recalculate
   * sector_start and sector_end etc.
   *
   */
  void UpdateSector() noexcept;

  /**
   * Test whether an angle is inside the sector limits
   *
   * @param that Angle to test (deg)
   *
   * @return True if that is within the start/end radials
   */
  [[gnu::pure]]
  bool IsAngleInSector(const Angle that) const noexcept;

public:
  /* virtual methods from class ObservationZone */
  bool IsInSector(const GeoPoint &location) const noexcept override;
  OZBoundary GetBoundary() const noexcept override;
  double ScoreAdjustment() const noexcept override;

  /* virtual methods from class ObservationZonePoint */
  bool Equals(const ObservationZonePoint &other) const noexcept override;
  std::unique_ptr<ObservationZonePoint> Clone(const GeoPoint &_reference) const noexcept override {
    return std::make_unique<SectorZone>(*this, _reference);
  }

  /* virtual methods from class CylinderZone */
  void SetRadius(double new_radius) noexcept override {
    CylinderZone::SetRadius(new_radius);
    UpdateSector();
  }
};
