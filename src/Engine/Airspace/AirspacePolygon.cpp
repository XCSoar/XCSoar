/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "AirspacePolygon.hpp"
#include "Geo/Flat/TaskProjection.hpp"
#include "Geo/Flat/FlatRay.hpp"
#include "AirspaceIntersectSort.hpp"
#include "AirspaceIntersectionVector.hpp"

#include <assert.h>

AirspacePolygon::AirspacePolygon(const std::vector<GeoPoint> &pts,
                                 const bool prune)
  :AbstractAirspace(Shape::POLYGON)
{
  if (pts.size() < 2) {
    m_is_convex = true;
  } else {
    m_border.reserve(pts.size() + 1);

    for (const GeoPoint &pt : pts)
      m_border.push_back(SearchPoint(pt));

    // ensure airspace is closed
    GeoPoint p_start = pts[0];
    GeoPoint p_end = *(pts.end() - 1);
    if (p_start != p_end)
      m_border.push_back(SearchPoint(p_start));


    if (prune) {
      // only for testing
      m_border.PruneInterior();
      m_is_convex = true;
    } else {
      m_is_convex = m_border.IsConvex();
    }
  }
}

const GeoPoint 
AirspacePolygon::GetCenter() const
{
  if (m_border.empty())
    return GeoPoint::Invalid();

  return m_border[0].GetLocation();
}

bool 
AirspacePolygon::Inside(const GeoPoint &loc) const
{
  return m_border.IsInside(loc);
}

AirspaceIntersectionVector
AirspacePolygon::Intersects(const GeoPoint &start, const GeoPoint &end,
                            const TaskProjection &projection) const
{
  const FlatRay ray(projection.ProjectInteger(start),
                    projection.ProjectInteger(end));

  AirspaceIntersectSort sorter(start, *this);

  for (auto it = m_border.begin(); it + 1 != m_border.end(); ++it) {

    const FlatRay r_seg(it->GetFlatLocation(), (it + 1)->GetFlatLocation());
    fixed t = ray.DistinctIntersection(r_seg);
    if (!negative(t))
      sorter.add(t, projection.Unproject(ray.Parametric(t)));
  }

  return sorter.all();
}

GeoPoint 
AirspacePolygon::ClosestPoint(const GeoPoint &loc,
                              const TaskProjection &projection) const
{
  const FlatGeoPoint p = projection.ProjectInteger(loc);
  const FlatGeoPoint pb = m_border.NearestPoint(p);
  return projection.Unproject(pb);
}
