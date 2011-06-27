/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Flat/FlatGeoPoint.hpp"
#include "Flat/FlatPoint.hpp"
#include "Flat/FlatBoundingBox.hpp"
#include "Geo/GeoBounds.hpp"
#include "Math/Earth.hpp"

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
TaskProjection::reset(const GeoPoint &ref) 
{
  location_min = ref;
  location_max = ref;
  location_mid = ref;

#ifndef NDEBUG
  initialised = true;
#endif
}

void TaskProjection::scan_location(const GeoPoint &ref) 
{
  assert(initialised);

  location_min.Longitude = min(ref.Longitude, location_min.Longitude);
  location_max.Longitude = max(ref.Longitude, location_max.Longitude);
  location_min.Latitude = min(ref.Latitude, location_min.Latitude);
  location_max.Latitude = max(ref.Latitude, location_max.Latitude);
}

bool
TaskProjection::update_fast()
{
  assert(initialised);

  GeoPoint old_loc = location_mid;

  location_mid.Longitude =
    location_max.Longitude.Fraction(location_min.Longitude, fixed_half);
  location_mid.Latitude =
    location_max.Latitude.Fraction(location_min.Latitude, fixed_half);
  cos_midloc = location_mid.Latitude.fastcosine() * fixed_scale;
  r_cos_midloc = fixed_one/cos_midloc;
  approx_scale = unproject(FlatGeoPoint(0,-1)).distance(unproject(FlatGeoPoint(0,1)))*fixed_half;

  return !(old_loc == location_mid);
}

FlatPoint
TaskProjection::fproject(const GeoPoint& tp) const
{
  assert(initialised);

  return FlatPoint((tp.Longitude - location_mid.Longitude)
                   .as_delta().value_native() * cos_midloc,
                   (tp.Latitude - location_mid.Latitude)
                   .as_delta().value_native() * fixed_scale);
}

GeoPoint 
TaskProjection::funproject(const FlatPoint& fp) const
{
  assert(initialised);

  GeoPoint tp;
  tp.Longitude = (Angle::native(fp.x*r_cos_midloc)+location_mid.Longitude).as_delta();
  tp.Latitude = (Angle::native(fp.y*inv_scale)+location_mid.Latitude).as_delta();
  return tp;
}

FlatGeoPoint 
TaskProjection::project(const GeoPoint& tp) const
{
  assert(initialised);

  FlatPoint f = fproject(tp);
  FlatGeoPoint fp;
  fp.Longitude = iround(f.x);
  fp.Latitude = iround(f.y);
  return fp;
}

GeoPoint 
TaskProjection::unproject(const FlatGeoPoint& fp) const
{
  assert(initialised);

  GeoPoint tp;
  tp.Longitude = Angle::native(fixed(fp.Longitude)*r_cos_midloc) + location_mid.Longitude;
  tp.Latitude = Angle::native(fixed(fp.Latitude)*inv_scale) + location_mid.Latitude;
  return tp;
}

fixed
TaskProjection::fproject_range(const GeoPoint &tp, const fixed range) const
{
  assert(initialised);

  GeoPoint fr = ::FindLatitudeLongitude(tp, Angle::native(fixed_zero), range);
  FlatPoint f = fproject(fr);
  FlatPoint p = fproject(tp);
  return fabs(f.y - p.y);
}

unsigned
TaskProjection::project_range(const GeoPoint &tp, const fixed range) const
{
  assert(initialised);

  return iround(fproject_range(tp, range));
}

fixed
TaskProjection::ApproxRadius() const
{
  assert(initialised);

  return max(location_mid.distance(location_max),
             location_mid.distance(location_min));
}

GeoBounds
TaskProjection::unproject(const FlatBoundingBox& bb) const
{
  assert(initialised);

  return GeoBounds (unproject(FlatGeoPoint(bb.bb_ll.Longitude, bb.bb_ur.Latitude)),
                    unproject(FlatGeoPoint(bb.bb_ur.Longitude, bb.bb_ll.Latitude)));
}

FlatBoundingBox
TaskProjection::project(const GeoBounds& bb) const
{
  assert(initialised);

  FlatBoundingBox fb(project(GeoPoint(bb.west, bb.south)),
                     project(GeoPoint(bb.east, bb.north)));
  fb.expand(); // prevent rounding
  return fb;
}

