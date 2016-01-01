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

#include "AirspaceCircle.hpp"
#include "Geo/GeoVector.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Geo/Flat/FlatLine.hpp"
#include "AirspaceIntersectSort.hpp"
#include "AirspaceIntersectionVector.hpp"

AirspaceCircle::AirspaceCircle(const GeoPoint &loc, const double _radius)
  :AbstractAirspace(Shape::CIRCLE), m_center(loc), m_radius(_radius)
{
  is_convex = TriState::TRUE;

  // @todo: find better enclosing radius as fn of NUM_SEGMENTS

  static constexpr unsigned NUM_SEGMENTS = 12;
  m_border.reserve(NUM_SEGMENTS);
  Angle angle = Angle::Zero();
  static constexpr Angle delta = Angle::FullCircle() / NUM_SEGMENTS;
  for (unsigned i = 0; i <= NUM_SEGMENTS; ++i, angle += delta) {
    const GeoPoint p = GeoVector(m_radius * 1.1, angle).EndPoint(m_center);
    m_border.emplace_back(p);
  }
}

bool
AirspaceCircle::Inside(const GeoPoint &loc) const
{
  return loc.DistanceS(m_center) <= m_radius;
}

AirspaceIntersectionVector
AirspaceCircle::Intersects(const GeoPoint &start, const GeoPoint &end,
                           const FlatProjection &projection) const
{
  const auto f_radius = projection.ProjectRangeFloat(m_center, m_radius);
  const auto f_center = projection.ProjectFloat(m_center);
  const auto f_start = projection.ProjectFloat(start);
  const auto f_end = projection.ProjectFloat(end);
  const FlatLine line(f_start, f_end);

  FlatPoint f_p1, f_p2;
  if (!line.IntersectCircle(f_radius, f_center, f_p1, f_p2))
    return AirspaceIntersectionVector();

  const auto mag = line.GetSquaredDistance();
  if (mag <= 0)
    return AirspaceIntersectionVector();

  const auto inv_mag = 1. / mag;
  const auto t1 = FlatLine(f_start, f_p1).DotProduct(line);
  const auto t2 = f_p1 == f_p2
    ? double(-1)
    : FlatLine(f_start, f_p2).DotProduct(line);

  const bool in_range = (t1 < mag) || (t2 < mag);
  // if at least one point is within range, capture both points

  AirspaceIntersectSort sorter(start, *this);
  if (t1 >= 0 && in_range)
    sorter.add(t1 * inv_mag, projection.Unproject(f_p1));

  if (t2 >= 0 && in_range)
    sorter.add(t2 * inv_mag, projection.Unproject(f_p2));

  return sorter.all();
}

GeoPoint
AirspaceCircle::ClosestPoint(const GeoPoint &loc,
                             gcc_unused const FlatProjection &projection) const
{
  // Calculate distance from center point
  const auto d = loc.DistanceS(m_center);

  // If loc is INSIDE the circle return loc itself
  if (d <= m_radius)
    return loc;

  // Otherwise calculate point on the circle in
  // the direction from center to loc
  return m_center.IntermediatePoint(loc, m_radius);
}
