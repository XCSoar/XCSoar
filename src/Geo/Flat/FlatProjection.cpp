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

#include "FlatProjection.hpp"
#include "FlatGeoPoint.hpp"
#include "FlatPoint.hpp"
#include "FlatBoundingBox.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/FAISphere.hpp"

#include <cassert>

// scaling for flat earth integer representation, gives approximately 50m resolution
static constexpr int fixed_scale = 57296;
static constexpr double inv_scale(1.0/fixed_scale);

void
FlatProjection::SetCenter(const GeoPoint &_center)
{
  assert(_center.IsValid());

  center = _center;

  cos = center.latitude.fastcosine() * fixed_scale;
  r_cos = 1. / cos;
  approx_scale = Unproject(FlatGeoPoint(0,-1)).DistanceS(Unproject(FlatGeoPoint(0,1))) / 2;
}

FlatPoint
FlatProjection::ProjectFloat(const GeoPoint &tp) const
{
  assert(IsValid());

  return FlatPoint((tp.longitude - center.longitude)
                   .AsDelta().Native() * cos,
                   (tp.latitude - center.latitude)
                   .AsDelta().Native() * fixed_scale);
}

GeoPoint
FlatProjection::Unproject(const FlatPoint &fp) const
{
  assert(IsValid());

  GeoPoint tp;
  tp.longitude = (Angle::Native(fp.x * r_cos) + center.longitude).AsDelta();
  tp.latitude = (Angle::Native(fp.y * inv_scale) + center.latitude).AsDelta();
  return tp;
}

FlatGeoPoint
FlatProjection::ProjectInteger(const GeoPoint &tp) const
{
  assert(IsValid());

  FlatPoint f = ProjectFloat(tp);
  return FlatGeoPoint(iround(f.x), iround(f.y));
}

GeoPoint
FlatProjection::Unproject(const FlatGeoPoint &fp) const
{
  assert(IsValid());

  return GeoPoint(Angle::Native(fp.x * r_cos)
                  + center.longitude,
                  Angle::Native(fp.y * inv_scale)
                  + center.latitude);
}

double
FlatProjection::ProjectRangeFloat(const GeoPoint &tp,
                                  const double range) const
{
  assert(IsValid());

  return FAISphere::EarthDistanceToAngle(range).Native() * fixed_scale;
}

unsigned
FlatProjection::ProjectRangeInteger(const GeoPoint &tp,
                                    const double range) const
{
  assert(IsValid());

  return iround(ProjectRangeFloat(tp, range));
}

GeoBounds
FlatProjection::Unproject(const FlatBoundingBox &bb) const
{
  assert(IsValid());

  return GeoBounds(Unproject(bb.GetTopLeft()), Unproject(bb.GetBottomRight()));
}

FlatBoundingBox
FlatProjection::Project(const GeoBounds &bb) const
{
  assert(IsValid());

  FlatBoundingBox fb(ProjectInteger(bb.GetSouthWest()),
                     ProjectInteger(bb.GetNorthEast()));
  fb.ExpandByOne(); // prevent rounding
  return fb;
}

FlatBoundingBox
FlatProjection::ProjectSquare(const GeoPoint center, double radius) const
{
  FlatGeoPoint flat_center = ProjectInteger(center);
  int flat_radius = ProjectRangeInteger(center, radius);
  return FlatBoundingBox(flat_center, flat_radius);
}

