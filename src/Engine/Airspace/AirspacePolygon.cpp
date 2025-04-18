// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspacePolygon.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Geo/Flat/FlatRay.hpp"
#include "AirspaceIntersectSort.hpp"
#include "AirspaceIntersectionVector.hpp"

AirspacePolygon::AirspacePolygon(const std::vector<GeoPoint> &pts) noexcept
  :AbstractAirspace(Shape::POLYGON)
{
  assert(pts.size() >= 3);

  m_border.reserve(pts.size() + 1);

  for (const GeoPoint &pt : pts)
    m_border.emplace_back(pt);

  // ensure airspace is closed
  const GeoPoint &p_start = pts.front();
  const GeoPoint &p_end = pts.back();
  if (p_start != p_end)
    m_border.emplace_back(p_start);

  is_convex = TriState::UNKNOWN;
}

const GeoPoint
AirspacePolygon::GetReferenceLocation() const noexcept
{
  assert(m_border.size() >= 3);

  return m_border[0].GetLocation();
}

const GeoPoint
AirspacePolygon::GetCenter() const noexcept
{
  assert(m_border.size() >= 3);

  double lat(0), lon(0);

  for (const auto &pt : m_border) {
    lat += pt.GetLocation().latitude.Native();
    lon += pt.GetLocation().longitude.Native();
  }

  lon = lon / m_border.size();
  lat = lat / m_border.size();

  return GeoPoint(Angle::Native(lon), Angle::Native(lat));
}

bool
AirspacePolygon::Inside(const GeoPoint &loc) const noexcept
{
  return m_border.IsInside(loc);
}

AirspaceIntersectionVector
AirspacePolygon::Intersects(const GeoPoint &start, const GeoPoint &end,
                            const FlatProjection &projection) const noexcept
{
  const FlatRay ray(projection.ProjectInteger(start),
                    projection.ProjectInteger(end));

  AirspaceIntersectSort sorter(start, *this);

  for (auto it = m_border.begin(); it + 1 != m_border.end(); ++it) {

    const FlatRay r_seg(it->GetFlatLocation(), (it + 1)->GetFlatLocation());
    auto t = ray.DistinctIntersection(r_seg);
    if (t >= 0)
      sorter.add(t, projection.Unproject(ray.Parametric(t)));
  }

  return sorter.all();
}

GeoPoint
AirspacePolygon::ClosestPoint(const GeoPoint &loc,
                              const FlatProjection &projection) const noexcept
{
  const auto p = projection.ProjectInteger(loc);
  const auto pb = m_border.NearestPoint(p);
  return projection.Unproject(pb);
}
