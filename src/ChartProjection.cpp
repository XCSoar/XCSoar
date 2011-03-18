/*
Copyright_License {

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

#include "ChartProjection.hpp"
#include "Engine/Task/TaskManager.hpp"

ChartProjection::ChartProjection(const RECT &rc,
                                 const TaskManager &task,
                                 const GeoPoint &fallback_loc)
{
  const GeoPoint center = task.get_task_center(fallback_loc);
  const fixed radius = max(fixed(10000), task.get_task_radius(fallback_loc));
  set_projection(rc, center, radius);
}

ChartProjection::ChartProjection(const RECT &rc,
                                 const OrderedTask& task,
                                 const GeoPoint &fallback_loc) 
{
  const GeoPoint center = task.get_task_center(fallback_loc);
  const fixed radius = max(fixed(10000), task.get_task_radius(fallback_loc));
  set_projection(rc, center, radius);
}

ChartProjection::ChartProjection(const RECT &rc,
                                 const TracePointVector& trace,
                                 const GeoPoint &fallback_loc) 
{
  const TaskProjection proj = get_bounds(trace, fallback_loc);
  const GeoPoint center = proj.get_center();
  const fixed radius = max(fixed(10000), proj.ApproxRadius() * fixed(1.1));
  set_projection(rc, center, radius);
}

void ChartProjection::set_projection(const RECT &rc, 
                                     const GeoPoint &center,
                                     const fixed radius)
{
  SetMapRect(rc);
  SetScale(fixed(GetScreenDistance()) / (radius * 2));
  SetGeoLocation(center);
  SetScreenOrigin((rc.left + rc.right) / 2, (rc.bottom + rc.top) / 2);
  UpdateScreenBounds();
}

ChartProjection::ChartProjection(const RECT &rc,
                                 const OrderedTaskPoint& point,
                                 const GeoPoint &fallback_loc)
{
  TaskProjection task_projection;
  task_projection.reset(fallback_loc);
  point.scan_projection(task_projection);

  const GeoPoint center = task_projection.get_center();
  const fixed radius = max(fixed(10000), task_projection.ApproxRadius());
  set_projection(rc, center, radius * fixed(1.3));
}
