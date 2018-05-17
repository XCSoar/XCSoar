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

#include "MapWindow.hpp"
#include "Geo/Math.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Renderer/TaskRenderer.hpp"
#include "Renderer/TaskPointRenderer.hpp"
#include "Renderer/OZRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Math/Screen.hpp"
#include "Look/MapLook.hpp"

void
MapWindow::DrawTask(Canvas &canvas)
{
  if (task == nullptr)
    return;

  /* RLD bearing is invalid if GPS not connected and in non-sim mode,
   but we can still draw targets */
  bool draw_bearing = Basic().track_available;
  bool draw_route = draw_bearing;

  if (draw_bearing) {
    if (Calculated().planned_route.size()>2) {
      draw_bearing = false;
    } else {
      draw_route = false;
    }
  }

  ProtectedTaskManager::Lease task_manager(*task);
  const AbstractTask *task = task_manager->GetActiveTask();
  if (task && task->CheckTask()) {
    TaskPointRenderer::TargetVisibility target_visibility =
        IsNearSelf() ? TaskPointRenderer::ACTIVE : TaskPointRenderer::ALL;

    OZRenderer ozv(look.task, airspace_renderer.GetLook(),
                   GetMapSettings().airspace);
    TaskPointRenderer tpv(canvas, render_projection, look.task,
                          /* we're accessing the OrderedTask here,
                             which may be invalid at this point, but it
                             will be used only if active, so it's ok */
                          task_manager->GetOrderedTask().GetTaskProjection(),
                          ozv, draw_bearing, target_visibility,
                          Basic().location_available
                          ? Basic().location : GeoPoint::Invalid());
    tpv.SetTaskFinished(Calculated().task_stats.task_finished);
    TaskRenderer dv(tpv, render_projection.GetScreenBounds());
    dv.Draw(*task);
  }

  if (draw_route)
    DrawRoute(canvas);
}

void
MapWindow::DrawRoute(Canvas &canvas)
{
  const auto &route = Calculated().planned_route;

  const auto r_size = route.size();
  BulkPixelPoint p[r_size], *pp = &p[0];
  for (auto i = route.begin(), end = route.end(); i != end; ++i, ++pp)
    *pp = render_projection.GeoToScreen(*i);

  p[r_size - 1] = ScreenClosestPoint(p[r_size-1], p[r_size-2], p[r_size-1], Layout::Scale(20));

  canvas.Select(look.task.bearing_pen);
  canvas.DrawPolyline(p, r_size);
}

void
MapWindow::DrawTaskOffTrackIndicator(Canvas &canvas)
{
  if (Calculated().circling 
      || !Basic().location_available
      || !Basic().track_available
      || !GetMapSettings().detour_cost_markers_enabled)
    return;

  const TaskStats &task_stats = Calculated().task_stats;
  const ElementStat &current_leg = task_stats.current_leg;

  if (!task_stats.task_valid || !current_leg.location_remaining.IsValid())
    return;

  const GeoPoint target = current_leg.location_remaining;
  GeoVector vec(Basic().location, target);

  if ((Basic().track - vec.bearing).AsDelta().AbsoluteDegrees() < 10)
    // insignificant error
    return;

  auto distance_max =
    std::min(vec.distance,
             render_projection.GetScreenDistanceMeters() * 0.7);

  // too short to bother
  if (distance_max < 5000)
    return;

  GeoPoint start = Basic().location;
  
  canvas.Select(*look.overlay.overlay_font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();
  
  GeoPoint dloc;
  int ilast = 0;
  for (double d = 0.25; d <= 1; d += 0.25) {
    dloc = FindLatitudeLongitude(start, Basic().track, distance_max * d);
    
    double distance0 = start.DistanceS(dloc);
    double distance1 = target.DistanceS(dloc);
    double distance = (distance0 + distance1) / vec.distance;
    int idist = iround((distance - 1) * 100);
    
    if ((idist != ilast) && (idist > 0) && (idist < 1000)) {
      TCHAR Buffer[5];
      _stprintf(Buffer, _T("%d"), idist);
      auto sc = render_projection.GeoToScreen(dloc);
      PixelSize tsize = canvas.CalcTextSize(Buffer);
      canvas.DrawText(sc.x - tsize.cx / 2, sc.y - tsize.cy / 2, Buffer);
      ilast = idist;
    }
  }
}
