// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbstractAirspace.hpp"
#include <vector>

#ifdef DO_PRINT
#include <iosfwd>
#endif

/** General polygon form airspace */
class AirspacePolygon final : public AbstractAirspace {
public:
  /**
   * Constructor.  For testing, pts vector is a cloud of points,
   * and the airspace polygon becomes the convex hull border.
   *
   * @param pts Vector representing border
   *
   * @return Initialised airspace object
   */
  explicit AirspacePolygon(const std::vector<GeoPoint> &pts) noexcept;

  /**
   * Converts border to convex hull of points (for testing only).
   */
  void MakeConvex() noexcept {
    m_border.PruneInterior();
    is_convex = TriState::TRUE;
  }

  /* virtual methods from class AbstractAirspace */
  const GeoPoint GetReferenceLocation() const noexcept override;
  const GeoPoint GetCenter() const noexcept override;
  bool Inside(const GeoPoint &loc) const noexcept override;
  AirspaceIntersectionVector Intersects(const GeoPoint &g1,
                                        const GeoPoint &end,
                                        const FlatProjection &projection) const noexcept override;
  GeoPoint ClosestPoint(const GeoPoint &loc,
                        const FlatProjection &projection) const noexcept override;

public:
#ifdef DO_PRINT
  friend std::ostream &operator<<(std::ostream &f,
                                  const AirspacePolygon &as);
#endif
};
