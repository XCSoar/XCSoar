// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskEditMapPreviewWindow.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Interface.hpp"
#include "Look/MapLook.hpp"
#include "Renderer/ActiveTaskGeometryRenderer.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Renderer/WaypointRenderer.hpp"
#include "UIGlobals.hpp"
#include "ui/canvas/Canvas.hpp"

#include <optional>
#include <variant>

void
TaskEditMapPreviewWindow::PaintOverlays(Canvas &canvas) noexcept
{
  LabelBlock label_block;
  label_block.reset();

  const WaypointLook &waypoint_look = UIGlobals::GetMapLook().waypoint;

  std::visit(
    [&](auto &&arg) noexcept {
      using T = std::decay_t<decltype(arg)>;

      if constexpr (std::is_same_v<T, MapPreviewFocusTaskTurnpoint>) {
        if (arg.task != nullptr && arg.index < arg.task->TaskSize()) {
          DrawActiveTaskForProjection(
            canvas, GetProjection(), task_look, *arg.task,
            arg.task->GetTaskProjection(),
            GetLayers().GetAirspaceRenderer(),
            CommonInterface::GetMapSettings(), false,
            TaskPointRenderer::TargetVisibility::ALL,
            CommonInterface::Basic().GetLocationOrInvalid(),
            false,
            arg.index);
          RenderOrderedTaskWaypointLabels(
            canvas, label_block, GetProjection(),
            CommonInterface::GetMapSettings().waypoint,
            waypoint_look, *arg.task);
        }
      } else if constexpr (std::is_same_v<T, MapPreviewFocusTaskWhole>) {
        if (arg.task != nullptr && arg.task->TaskSize() > 0) {
          DrawActiveTaskForProjection(
            canvas, GetProjection(), task_look, *arg.task,
            arg.task->GetTaskProjection(),
            GetLayers().GetAirspaceRenderer(),
            CommonInterface::GetMapSettings(), false,
            TaskPointRenderer::TargetVisibility::ALL,
            CommonInterface::Basic().GetLocationOrInvalid(),
            false,
            std::nullopt);
          RenderOrderedTaskWaypointLabels(
            canvas, label_block, GetProjection(),
            CommonInterface::GetMapSettings().waypoint,
            waypoint_look, *arg.task);
        }
      } else {
        (void)arg;
      }
    },
    GetFocus());
}
