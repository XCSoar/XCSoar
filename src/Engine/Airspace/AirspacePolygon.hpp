/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef AIRSPACEPOLYGON_HPP
#define AIRSPACEPOLYGON_HPP

#include "AbstractAirspace.hpp"
#include <vector>

#ifdef DO_PRINT
#include <iostream>
#endif

/** General polygon form airspace */
class AirspacePolygon final : public AbstractAirspace {
public:
  /**
   * Constructor.  For testing, pts vector is a cloud of points,
   * and the airspace polygon becomes the convex hull border.
   *
   * @param pts Vector representing border
   * @param prune If true, converts border to convex hull of points (for testing only)
   *
   * @return Initialised airspace object
   */
  AirspacePolygon(const std::vector<GeoPoint> &pts, const bool prune = false);

  /* virtual methods from class AbstractAirspace */
  const GeoPoint GetReferenceLocation() const override;
  const GeoPoint GetCenter() const override;
  bool Inside(const GeoPoint &loc) const override;
  AirspaceIntersectionVector Intersects(const GeoPoint &g1,
                                        const GeoPoint &end,
                                        const FlatProjection &projection) const override;
  GeoPoint ClosestPoint(const GeoPoint &loc,
                        const FlatProjection &projection) const override;

public:
#ifdef DO_PRINT
  friend std::ostream &operator<<(std::ostream &f,
                                  const AirspacePolygon &as);
#endif
};

#endif
