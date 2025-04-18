// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbstractAirspace.hpp"

#ifdef DO_PRINT
#include <iosfwd>
#endif

/**
 * Airspace object defined by the area within a distance (radius) from
 * a center point.
 */
class AirspaceCircle final : public AbstractAirspace {
  const GeoPoint m_center;
  const double m_radius;

public:
  /**
   * Constructor
   *
   * @param loc Center point of circle
   * @param _radius Radius in meters of airspace boundary
   *
   * @return Initialised airspace object
   */
  AirspaceCircle(const GeoPoint &loc, const double _radius) noexcept;

  /* virtual methods from class AbstractAirspace */
  const GeoPoint GetReferenceLocation() const noexcept override {
    return m_center;
  }

  const GeoPoint GetCenter() const noexcept override {
    return GetReferenceLocation();
  }

  bool Inside(const GeoPoint &loc) const noexcept override;
  AirspaceIntersectionVector Intersects(const GeoPoint &g1,
                                        const GeoPoint &end,
                                        const FlatProjection &projection) const noexcept override;
  GeoPoint ClosestPoint(const GeoPoint &loc,
                        const FlatProjection &projection) const noexcept override;

  /**
   * Accessor for radius
   *
   * @return Radius of circle (m)
   */
  const double &GetRadius() const noexcept {
    return m_radius;
  }

public:
#ifdef DO_PRINT
  friend std::ostream &operator<<(std::ostream &f,
                                  const AirspaceCircle &as);
#endif
};
