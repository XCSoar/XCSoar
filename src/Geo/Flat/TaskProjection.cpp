/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "TaskProjection.hpp"
#include "FlatGeoPoint.hpp"
#include "FlatPoint.hpp"
#include "FlatBoundingBox.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/Math.hpp"

#include <algorithm>
#include <cassert>

// scaling for flat earth integer representation, gives approximately 50m resolution
#ifdef RADIANS
static const int fixed_scale = 57296;
#else
static const int fixed_scale = 1000;
#endif
static const fixed inv_scale(1.0/fixed_scale);

void
TaskProjection::Reset(const GeoPoint &ref)
{
  location_min = ref;
  location_max = ref;
  location_mid = ref;

#ifndef NDEBUG
  initialised = true;
#endif
}

void
TaskProjection::Scan(const GeoPoint &ref)
{
  assert(initialised);

  if (!ref.IsValid())
    return;

  location_min.longitude = min(ref.longitude, location_min.longitude);
  location_max.longitude = max(ref.longitude, location_max.longitude);
  location_min.latitude = min(ref.latitude, location_min.latitude);
  location_max.latitude = max(ref.latitude, location_max.latitude);
}

bool
TaskProjection::Update()
{
  assert(initialised);

  GeoPoint old_loc = location_mid;

  location_mid.longitude =
    location_max.longitude.Fraction(location_min.longitude, fixed(0.5));
  location_mid.latitude =
    location_max.latitude.Fraction(location_min.latitude, fixed(0.5));
  cos_midloc = location_mid.latitude.fastcosine() * fixed_scale;
  r_cos_midloc = fixed(1)/cos_midloc;
  approx_scale = Unproject(FlatGeoPoint(0,-1)).Distance(Unproject(FlatGeoPoint(0,1))) / 2;

  return !(old_loc == location_mid);
}

FlatPoint
TaskProjection::ProjectFloat(const GeoPoint& tp) const
{
  assert(initialised);

  return FlatPoint((tp.longitude - location_mid.longitude)
                   .AsDelta().Native() * cos_midloc,
                   (tp.latitude - location_mid.latitude)
                   .AsDelta().Native() * fixed_scale);
}

GeoPoint 
TaskProjection::Unproject(const FlatPoint& fp) const
{
  assert(initialised);

  GeoPoint tp;
  tp.longitude = (Angle::Native(fp.x*r_cos_midloc)+location_mid.longitude).AsDelta();
  tp.latitude = (Angle::Native(fp.y*inv_scale)+location_mid.latitude).AsDelta();
  return tp;
}

FlatGeoPoint 
TaskProjection::ProjectInteger(const GeoPoint& tp) const
{
  assert(initialised);

  FlatPoint f = ProjectFloat(tp);
  return FlatGeoPoint(iround(f.x), iround(f.y));
}

GeoPoint 
TaskProjection::Unproject(const FlatGeoPoint& fp) const
{
  assert(initialised);

  return GeoPoint(Angle::Native(fp.longitude * r_cos_midloc)
                  + location_mid.longitude,
                  Angle::Native(fp.latitude * inv_scale)
                  + location_mid.latitude);
}

fixed
TaskProjection::ProjectRangeFloat(const GeoPoint &tp, const fixed range) const
{
  assert(initialised);

  GeoPoint fr = ::FindLatitudeLongitude(tp, Angle::Zero(), range);
  FlatPoint f = ProjectFloat(fr);
  FlatPoint p = ProjectFloat(tp);
  return fabs(f.y - p.y);
}

unsigned
TaskProjection::ProjectRangeInteger(const GeoPoint &tp, const fixed range) const
{
  assert(initialised);

  return iround(ProjectRangeFloat(tp, range));
}

fixed
TaskProjection::ApproxRadius() const
{
  assert(initialised);

  return max(location_mid.Distance(location_max),
             location_mid.Distance(location_min));
}

GeoBounds
TaskProjection::Unproject(const FlatBoundingBox& bb) const
{
  assert(initialised);

  return GeoBounds(Unproject(FlatGeoPoint(bb.bb_ll.longitude,
                                          bb.bb_ur.latitude)),
                   Unproject(FlatGeoPoint(bb.bb_ur.longitude,
                                          bb.bb_ll.latitude)));
}

FlatBoundingBox
TaskProjection::Project(const GeoBounds& bb) const
{
  assert(initialised);

  FlatBoundingBox fb(ProjectInteger(GeoPoint(bb.west, bb.south)),
                     ProjectInteger(GeoPoint(bb.east, bb.north)));
  fb.ExpandByOne(); // prevent rounding
  return fb;
}

