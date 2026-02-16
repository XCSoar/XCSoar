// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
MapWindow::DrawTask(Canvas &canvas) noexcept
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
  if (task && !IsError(task->CheckTask())) {
    const auto target_visibility = IsNearSelf()
      ? TaskPointRenderer::TargetVisibility::ACTIVE
      : TaskPointRenderer::TargetVisibility::ALL;

    /* the FlatProjection parameter is used by class TaskPointRenderer
       only when drawing an OrderedTask, so it's okay to pass an
       uninitialized dummy reference when this is not an
       OrderedTask */
    const FlatProjection dummy_flat_projection{};
    const auto &flat_projection = task->GetType() == TaskType::ORDERED
      ? ((const OrderedTask *)task)->GetTaskProjection()
      : dummy_flat_projection;

    OZRenderer ozv(look.task, airspace_renderer.GetLook(),
                   GetMapSettings().airspace);
    TaskPointRenderer tpv(canvas, render_projection, look.task,
                          flat_projection,
                          ozv, draw_bearing, target_visibility,
                          Basic().GetLocationOrInvalid());
    tpv.SetTaskFinished(Calculated().task_stats.task_finished);
    TaskRenderer dv(tpv, render_projection.GetScreenBounds());
    dv.Draw(*task);
  }

  if (draw_route)
    DrawRoute(canvas);
}

void
MapWindow::DrawRoute(Canvas &canvas) noexcept
{
  const auto &route = Calculated().planned_route;

  const auto r_size = route.size();
  constexpr std::size_t capacity = std::decay_t<decltype(route)>::capacity();
  BulkPixelPoint p[capacity];
  std::transform(route.begin(), route.end(), p,
                 [this](const auto &i) {
                   return render_projection.GeoToScreen(i);
                 });

  p[r_size - 1] = ScreenClosestPoint(p[r_size-1], p[r_size-2], p[r_size-1], Layout::Scale(20));

  canvas.Select(look.task.bearing_pen);
  canvas.DrawPolyline(p, r_size);
}

void
MapWindow::DrawTaskOffTrackIndicator(Canvas &canvas) noexcept
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

  if ((Basic().track - vec.bearing).AsDelta().Absolute() < Angle::Degrees(10))
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
      char Buffer[5];
      _stprintf(Buffer, _T("%d"), idist);
      auto sc = render_projection.GeoToScreen(dloc);
      PixelSize tsize = canvas.CalcTextSize(Buffer);
      canvas.DrawText(sc - tsize / 2u, Buffer);
      ilast = idist;
    }
  }
}
