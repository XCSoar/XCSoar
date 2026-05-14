// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ActiveTaskGeometryRenderer.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "Renderer/OZRenderer.hpp"
#include "Renderer/TaskRenderer.hpp"
#include "Engine/Task/AbstractTask.hpp"
#include "MapSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"

void
DrawActiveTaskForProjection(Canvas &canvas,
                           const WindowProjection &projection,
                           const TaskLook &task_look,
                           const AbstractTask &task,
                           const FlatProjection &flat_projection,
                           const AirspaceRenderer &airspace_renderer,
                           const MapSettings &map_settings,
                           bool draw_bearing,
                           TaskPointRenderer::TargetVisibility target_visibility,
                           GeoPoint reference_location,
                           bool task_finished,
                           std::optional<unsigned> active_index) noexcept
{
  OZRenderer ozv(task_look, airspace_renderer.GetLook(),
                 map_settings.airspace);
  TaskPointRenderer tpv(canvas, projection, task_look,
                        flat_projection,
                        ozv, draw_bearing,
                        target_visibility,
                        reference_location);
  if (active_index.has_value())
    tpv.SetActiveIndex(*active_index);

  tpv.SetTaskFinished(task_finished);

  TaskRenderer dv(tpv, projection.GetScreenBounds());
  dv.Draw(task);
}
