/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
