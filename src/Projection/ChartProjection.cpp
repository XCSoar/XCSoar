/*
Copyright_License {

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

#include "ChartProjection.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"

void
ChartProjection::Set(const PixelRect &rc,
                     const TaskProjection &task_projection,
                     double radius_factor)
{
  const GeoPoint center = task_projection.GetCenter();
  const auto radius = std::max(double(10000),
                               task_projection.ApproxRadius() * radius_factor);
  Set(rc, center, radius);
}

void
ChartProjection::Set(const PixelRect &rc, const OrderedTask &task,
                     const GeoPoint &fallback_loc)
{
  GeoPoint center = task.GetTaskCenter();
  if (!center.IsValid())
    center = fallback_loc;

  const auto radius = std::max(double(10000), task.GetTaskRadius());
  Set(rc, center, radius);
}

void
ChartProjection::Set(const PixelRect &rc, const OrderedTaskPoint &point,
                     const GeoPoint &fallback_loc)
{
  GeoBounds bounds(fallback_loc);
  point.ScanBounds(bounds);

  Set(rc, TaskProjection(bounds), 1.3);
}

void
ChartProjection::Set(const PixelRect &rc, const GeoPoint &center,
                     const double radius)
{
  SetMapRect(rc);
  SetScaleFromRadius(radius);
  SetGeoLocation(center);
  SetScreenOrigin((rc.left + rc.right) / 2, (rc.bottom + rc.top) / 2);
  UpdateScreenBounds();
}
