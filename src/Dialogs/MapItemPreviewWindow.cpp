// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapItemPreviewWindow.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Geo/GeoBounds.hpp"
#include "Interface.hpp"
#include "Look/MapLook.hpp"
#include "Look/TrafficLook.hpp"
#include "Look/WaypointLook.hpp"
#include "MapWindow/Preview/MapPreviewFit.hpp"
#include "MapSettings.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/ActiveTaskGeometryRenderer.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Renderer/TrafficRenderer.hpp"
#include "Renderer/WaypointRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/TaskPreviewValidation.hpp"
#include "Task/WaypointInActiveTask.hpp"
#include "UIGlobals.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Pen.hpp"

#include <optional>
#include <variant>

void
MapItemPreviewWindow::UpdateProjection() noexcept
{
  const PixelRect rc = GetClientRect();
  if (rc.GetWidth() < 2 || rc.GetHeight() < 2)
    return;

  const auto *oz = std::get_if<MapPreviewFocusTaskOZ>(&GetFocus());
  if (oz != nullptr) {
    MapWindowProjection &projection = GetProjection();
    projection.SetMapRect(rc);
    projection.SetScreenOrigin(rc.GetCenter());

    if (backend_components == nullptr ||
        backend_components->protected_task_manager == nullptr) {
      ApplyFallbackProjection(rc);
    } else {
      ProtectedTaskManager::Lease lease(
        *backend_components->protected_task_manager);
      const OrderedTask &ot = lease->GetOrderedTask();
      if (oz->index >= ot.TaskSize())
        ApplyFallbackProjection(rc);
      else {
        const OrderedTaskPoint &tp = ot.GetTaskPoint(oz->index);
        const GeoBounds bounds = MapPreviewFit::TaskPointFullBounds(tp);
        if (bounds.IsValid())
          MapPreviewFit::GeoBoundsToProjectionCentered(projection, bounds,
                                                       tp.GetLocation(), rc);
        else
          MapPreviewFit::PointNeighbourhood(projection, tp.GetLocation(),
                                            8000., rc);
      }
    }

    projection.SetScreenAngle(Angle::Zero());
    projection.UpdateScreenBounds();
    return;
  }

  MapPreviewWindow::UpdateProjection();
}

void
MapItemPreviewWindow::PaintOverlays(Canvas &canvas) noexcept
{
  LabelBlock label_block;
  label_block.reset();

  std::visit(
    [&](auto &&arg) noexcept {
      using T = std::decay_t<decltype(arg)>;

      if constexpr (std::is_same_v<T, ConstAirspacePtr>) {
        if (arg)
          AirspaceRenderer::DrawOutlineHighlight(
            canvas, GetProjection(), *arg,
            CommonInterface::GetMapSettings().airspace);
      } else if constexpr (std::is_same_v<T, WaypointPtr>) {
        if (!arg)
          return;

        const auto &map_settings = CommonInterface::GetMapSettings();
        const auto &computer = CommonInterface::GetComputerSettings();
        RenderWaypointForMapPreview(canvas, label_block, GetProjection(),
                                    map_settings.waypoint, waypoint_look,
                                    computer.task, computer.polar,
                                    CommonInterface::Basic(),
                                    CommonInterface::Calculated(), arg,
                                    WaypointPtrInActiveOrderedTask(arg));
      } else if constexpr (std::is_same_v<T, MapPreviewFocusLocationLeg>) {
        const GeoPoint &target = arg.target;
        auto &projection = GetProjection();
        const MoreData &basic = CommonInterface::Basic();

        if (basic.location_available && target.IsValid()) {
          const PixelPoint from = projection.GeoToScreen(basic.location);
          const PixelPoint to = projection.GeoToScreen(target);
          canvas.Select(task_look.bearing_pen);
          canvas.DrawLine(from, to);

          AircraftRenderer::Draw(
            canvas, CommonInterface::GetMapSettings(),
            UIGlobals::GetMapLook().aircraft,
            basic.attitude.heading - projection.GetScreenAngle(),
            from);
        }

        if (target.IsValid()) {
          const Pen pen(Layout::ScalePenWidth(2), COLOR_BLACK);
          canvas.Select(pen);
          canvas.DrawCircle(projection.GeoToScreen(target),
                            Layout::Scale(4));
        }
      } else if constexpr (std::is_same_v<T, MapPreviewFocusTraffic>) {
        const FlarmTraffic *tr =
          CommonInterface::Basic().flarm.traffic.FindTraffic(arg.id);
        if (tr == nullptr || !tr->location_available)
          return;

        const PixelPoint pt = GetProjection().GeoToScreen(tr->location);
        const Angle bearing = tr->track_received ? Angle(tr->track) : Angle::Zero();
        TrafficRenderer::Draw(canvas, traffic_look, false, *tr, bearing,
                              arg.color, pt);
      } else if constexpr (std::is_same_v<T, MapPreviewFocusTaskOZ>) {
        if (backend_components == nullptr ||
            backend_components->protected_task_manager == nullptr)
          return;

        ProtectedTaskManager::Lease lease(
          *backend_components->protected_task_manager);
        const OrderedTask &ot = lease->GetOrderedTask();
        if (arg.index < ot.TaskSize() &&
            IsLiveTaskPreviewValid(&ot)) {
          DrawActiveTaskForProjection(
            canvas, GetProjection(), task_look, ot,
            ot.GetTaskProjection(),
            GetLayers().GetAirspaceRenderer(),
            CommonInterface::GetMapSettings(), false,
            TaskPointRenderer::TargetVisibility::ALL,
            CommonInterface::Basic().GetLocationOrInvalid(),
            CommonInterface::Calculated().task_stats.task_finished,
            arg.index);
          RenderOrderedTaskWaypointLabels(
            canvas, label_block, GetProjection(),
            CommonInterface::GetMapSettings().waypoint,
            waypoint_look, ot);
        }
      } else {
        (void)arg;
      }
    },
    GetFocus());
}
