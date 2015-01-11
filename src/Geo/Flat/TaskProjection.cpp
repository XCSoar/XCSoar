/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include <algorithm>
#include <cassert>

void
TaskProjection::Reset(const GeoPoint &ref)
{
  FlatProjection::SetInvalid();
  location_min = ref;
  location_max = ref;
}

void
TaskProjection::Scan(const GeoPoint &ref)
{
  assert(location_min.IsValid());
  assert(location_max.IsValid());

  if (!ref.IsValid())
    return;

  location_min.longitude = std::min(ref.longitude, location_min.longitude);
  location_max.longitude = std::max(ref.longitude, location_max.longitude);
  location_min.latitude = std::min(ref.latitude, location_min.latitude);
  location_max.latitude = std::max(ref.latitude, location_max.latitude);
}

bool
TaskProjection::Update()
{
  assert(location_min.IsValid());
  assert(location_max.IsValid());

  GeoPoint old_center = GetCenter();
  GeoPoint new_center;

  new_center.longitude =
    location_max.longitude.Fraction(location_min.longitude, fixed(0.5));
  new_center.latitude =
    location_max.latitude.Fraction(location_min.latitude, fixed(0.5));
  if (new_center == old_center)
    return false;

  SetCenter(new_center);
  return true;
}

fixed
TaskProjection::ApproxRadius() const
{
  assert(location_min.IsValid());
  assert(location_max.IsValid());

  return std::max(GetCenter().Distance(location_max),
                  GetCenter().Distance(location_min));
}
