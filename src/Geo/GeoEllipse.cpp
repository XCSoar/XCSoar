// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GeoEllipse.hpp"

GeoEllipse::GeoEllipse(const GeoPoint &f1, const GeoPoint &f2,
                       const GeoPoint &p, const FlatProjection &_projection)
  :projection(_projection),
   ell(projection.ProjectFloat(f1),
       projection.ProjectFloat(f2),
       projection.ProjectFloat(p))
{
}

GeoPoint 
GeoEllipse::Parametric(const double t) const
{
  const FlatPoint fp = ell.Parametric(t);
  return projection.Unproject(fp);
}

std::optional<std::pair<GeoPoint, GeoPoint>>
GeoEllipse::IntersectExtended(const GeoPoint &p) const noexcept
{
  const FlatPoint pf = projection.ProjectFloat(p);

  if (auto i = ell.IntersectExtended(pf))
    return std::pair<GeoPoint, GeoPoint>{
      projection.Unproject(i->first),
      projection.Unproject(i->second),
    };

  return std::nullopt;
}
