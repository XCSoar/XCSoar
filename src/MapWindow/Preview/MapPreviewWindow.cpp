// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapPreviewWindow.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Task/AbstractTask.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/ObservationZones/ObservationZonePoint.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/Task/TaskType.hpp"
#include "Task/TaskPreviewValidation.hpp"
#include "Geo/GeoBounds.hpp"
#include "Look/MapLook.hpp"
#include "MapSettings.hpp"
#include "MapPreviewFit.hpp"
#include "FLARM/List.hpp"
#include "Interface.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/ActiveTaskGeometryRenderer.hpp"
#include "Renderer/PlannedRouteRenderer.hpp"
#include "Renderer/WaypointRenderer.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "ui/canvas/Canvas.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#endif

#include <cmath>
#include <variant>

namespace {

inline void
DrawPreviewTask(Canvas &canvas,
                const MapWindowProjection &projection,
                const TaskLook &task_look,
                AirspaceRenderer &airspace_renderer,
                ProtectedTaskManager *task_manager,
                const MoreData &basic,
                const DerivedInfo &calculated,
                const MapSettings &map_settings) noexcept
{
  if (task_manager == nullptr)
    return;

  bool draw_bearing = basic.track_available;
  bool draw_route = draw_bearing;

  if (draw_bearing) {
    if (calculated.planned_route.size() > 2) {
      draw_bearing = false;
    } else {
      draw_route = false;
    }
  }

  ProtectedTaskManager::Lease lease(*task_manager);
  const AbstractTask *active_task = lease->GetActiveTask();
  if (IsLiveTaskPreviewValid(active_task)) {
    const FlatProjection dummy_flat_projection{};
    const auto &flat_projection =
      active_task->GetType() == TaskType::ORDERED
        ? ((const OrderedTask *)active_task)->GetTaskProjection()
        : dummy_flat_projection;

    DrawActiveTaskForProjection(canvas, projection, task_look,
                                *active_task,
                                flat_projection, airspace_renderer,
                                map_settings, draw_bearing,
                                TaskPointRenderer::TargetVisibility::ALL,
                                basic.GetLocationOrInvalid(),
                                calculated.task_stats.task_finished);
  }

  if (draw_route)
    DrawPlannedRoutePolyline(canvas, projection, task_look.bearing_pen,
                             calculated.planned_route);
}

} // namespace

MapPreviewWindow::MapPreviewWindow(const AirspaceLook &airspace_look,
                                   const TopographyLook &topography_look,
                                   ProtectedAirspaceWarningManager
                                     *warnings_manager
                                   ) noexcept
  : MapPreviewBufferWindow(airspace_look, topography_look),
    rate_limited_bl(static_cast<BlackboardListener &>(*this),
                    std::chrono::milliseconds(400),
                    std::chrono::milliseconds(120))
{
  if (warnings_manager != nullptr)
    layers.SetAirspaceWarnings(warnings_manager);
}

MapPreviewWindow::~MapPreviewWindow() noexcept = default;

void
MapPreviewWindow::Create(ContainerWindow &parent, PixelRect rc,
                         WindowStyle style) noexcept
{
  projection.SetScreenAngle(Angle::Zero());
  BufferWindow::Create(parent, rc, style);
}

void
MapPreviewWindow::SetPreviewFocus(MapPreviewFocus f) noexcept
{
  focus = std::move(f);
  SyncPreviewBlackboardListener();
  /* #Prepare runs top-down; list widgets may update focus before the preview
     #Create() — #Invalidate requires #IsDefined(). */
  if (IsDefined())
    Invalidate();
}

void
MapPreviewWindow::SetQueryFallbackLocation(GeoPoint location) noexcept
{
  query_fallback_location = location;
}

void
MapPreviewWindow::ApplyFallbackProjection(const PixelRect &rc) noexcept
{
  (void)rc;
  GeoPoint loc = query_fallback_location;
  if (!loc.IsValid())
    loc = CommonInterface::Basic().GetLocationOrInvalid();
  if (!loc.IsValid())
    loc = GeoPoint::Zero();

  projection.SetGeoLocation(loc);
  projection.SetScaleFromRadius(25000.);
}

bool
MapPreviewWindow::HasAirspaceMovingMapChrome() const noexcept
{
  if (airspace_preview_map_look == nullptr)
    return false;

  const auto *asp = std::get_if<ConstAirspacePtr>(&focus);
  return asp != nullptr && *asp != nullptr;
}

void
MapPreviewWindow::PaintAirspaceMovingMap(Canvas &canvas) noexcept
{
  if (!HasAirspaceMovingMapChrome())
    return;

  label_block.reset();

  const MapSettings &map_settings = CommonInterface::GetMapSettings();
  const auto &computer = CommonInterface::GetComputerSettings();
  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  ProtectedTaskManager *task_mgr =
    backend_components != nullptr
      ? backend_components->protected_task_manager.get()
      : nullptr;

  DrawPreviewTask(canvas, projection, airspace_preview_map_look->task,
                  layers.GetAirspaceRenderer(), task_mgr,
                  basic, calculated, map_settings);

  Waypoints *waypoints =
    data_components != nullptr ? data_components->waypoints.get() : nullptr;
  WaypointRenderer waypoint_renderer(waypoints,
                                     airspace_preview_map_look->waypoint);
  waypoint_renderer.Render(canvas, label_block, projection,
                           map_settings.waypoint,
                           computer.polar, computer.task,
                           basic, calculated,
                           task_mgr, nullptr);

  layers.RenderTopographyLabels(canvas, projection, label_block);

  /* Same gate as #MapWindowRender: draw own ship whenever fix is available. */
  if (basic.location_available) {
    const PixelPoint aircraft_pos =
      projection.GeoToScreen(basic.location);
    AircraftRenderer::Draw(canvas, map_settings,
                           airspace_preview_map_look->aircraft,
                           basic.attitude.heading -
                             projection.GetScreenAngle(),
                           aircraft_pos);
  }
}

void
MapPreviewWindow::UpdateProjection() noexcept
{
  const PixelRect rc = GetClientRect();
  if (rc.GetWidth() < 2 || rc.GetHeight() < 2)
    return;

  projection.SetMapRect(rc);
  projection.SetScreenOrigin(rc.GetCenter());

  std::visit(
    [&](auto &&arg) noexcept {
      using T = std::decay_t<decltype(arg)>;

      if constexpr (std::is_same_v<T, std::monostate>) {
        ApplyFallbackProjection(rc);
      } else if constexpr (std::is_same_v<T, ConstAirspacePtr>) {
        const AbstractAirspace *ap = arg.get();
        if (ap == nullptr) {
          ApplyFallbackProjection(rc);
          return;
        }

        const GeoBounds bounds = ap->GetGeoBounds();
        if (bounds.IsValid())
          MapPreviewFit::GeoBoundsToProjection(projection, bounds, rc);
        else
          ApplyFallbackProjection(rc);
      } else if constexpr (std::is_same_v<T, WaypointPtr>) {
        if (!arg) {
          ApplyFallbackProjection(rc);
          return;
        }

        MapPreviewFit::PointNeighbourhood(projection, arg->location,
                                          4500., rc);
      } else if constexpr (std::is_same_v<T, MapPreviewFocusLocationLeg>) {
        const GeoPoint &target = arg.target;
        if (!target.IsValid()) {
          ApplyFallbackProjection(rc);
          return;
        }

        const MoreData &basic = CommonInterface::Basic();
        if (basic.location_available) {
          GeoBounds bounds(basic.location);
          bounds.Extend(target);
          if (bounds.IsValid())
            MapPreviewFit::GeoBoundsToProjection(projection, bounds, rc);
          else
            MapPreviewFit::PointNeighbourhood(projection, target, 4500., rc);
        } else
          MapPreviewFit::PointNeighbourhood(projection, target, 4500., rc);
      } else if constexpr (std::is_same_v<T, GeoPoint>) {
        if (!arg.IsValid()) {
          ApplyFallbackProjection(rc);
          return;
        }

        MapPreviewFit::PointNeighbourhood(projection, arg, 4500., rc);
      } else if constexpr (std::is_same_v<T, MapPreviewFocusTraffic>) {
        const FlarmTraffic *tr =
          CommonInterface::Basic().flarm.traffic.FindTraffic(arg.id);
        GeoPoint loc = query_fallback_location;
        if (tr != nullptr && tr->location_available && tr->location.IsValid())
          loc = tr->location;
        else if (!loc.IsValid())
          loc = CommonInterface::Basic().GetLocationOrInvalid();
        if (!loc.IsValid())
          loc = GeoPoint::Zero();

        projection.SetGeoLocation(loc);
        projection.SetScaleFromRadius(9000.);
      } else if constexpr (std::is_same_v<T, MapPreviewFocusTaskOZ>) {
        /* Resolved in #MapItemPreviewWindow::UpdateProjection — uses task lease. */
        ApplyFallbackProjection(rc);
      } else if constexpr (std::is_same_v<T, MapPreviewFocusTaskTurnpoint>) {
        OrderedTask *task = arg.task;
        if (task == nullptr || arg.index >= task->TaskSize()) {
          ApplyFallbackProjection(rc);
          return;
        }

        const OrderedTaskPoint &tp = task->GetTaskPoint(arg.index);
        const GeoBounds bounds = MapPreviewFit::TaskPointFullBounds(tp);
        if (bounds.IsValid())
          MapPreviewFit::GeoBoundsToProjectionCentered(projection, bounds,
                                                       tp.GetLocation(), rc);
        else
          MapPreviewFit::PointNeighbourhood(projection, tp.GetLocation(),
                                            8000., rc);
      } else if constexpr (std::is_same_v<T, MapPreviewFocusTaskWhole>) {
        const OrderedTask *task = arg.task;
        if (task == nullptr || task->TaskSize() == 0) {
          ApplyFallbackProjection(rc);
          return;
        }

        GeoBounds tb = task->GetTaskProjection().GetBounds();
        if (!tb.IsValid()) {
          ApplyFallbackProjection(rc);
          return;
        }

        MapPreviewFit::GeoBoundsToProjection(projection, tb, rc);
      }
    },
    focus);

  projection.SetScreenAngle(Angle::Zero());
  projection.UpdateScreenBounds();
}

void
MapPreviewWindow::LinkDataSources() noexcept
{
  layers.LinkFromDataSources();
}

void
MapPreviewWindow::SyncPreviewBlackboardListener() noexcept
{
  const bool want =
    std::holds_alternative<MapPreviewFocusTraffic>(focus) ||
    std::holds_alternative<MapPreviewFocusLocationLeg>(focus) ||
    HasAirspaceMovingMapChrome();

  if (want && !blackboard_registered) {
    CommonInterface::AddListener(rate_limited_bl);
    blackboard_registered = true;
  } else if (!want && blackboard_registered) {
    CommonInterface::RemoveListener(rate_limited_bl);
    rate_limited_bl.Cancel();
    blackboard_registered = false;
  }
}

void
MapPreviewWindow::OnGPSUpdate(const MoreData &) noexcept
{
  const bool traffic = std::holds_alternative<MapPreviewFocusTraffic>(focus);
  const bool location_leg =
    std::holds_alternative<MapPreviewFocusLocationLeg>(focus);
  const bool airspace_chrome = HasAirspaceMovingMapChrome();
  if ((traffic || location_leg || airspace_chrome) && IsDefined())
    Invalidate();
}

void
MapPreviewWindow::OnCalculatedUpdate(const MoreData &,
                                   const DerivedInfo &) noexcept
{
  const bool traffic = std::holds_alternative<MapPreviewFocusTraffic>(focus);
  const bool location_leg =
    std::holds_alternative<MapPreviewFocusLocationLeg>(focus);
  const bool airspace_chrome = HasAirspaceMovingMapChrome();
  if ((traffic || location_leg || airspace_chrome) && IsDefined())
    Invalidate();
}

void
MapPreviewWindow::OnCreate() noexcept
{
  BufferWindow::OnCreate();

  InitialiseAirspaceBuffer();

  if (IsDefined())
    Invalidate();
}

void
MapPreviewWindow::OnDestroy() noexcept
{
  if (blackboard_registered) {
    CommonInterface::RemoveListener(rate_limited_bl);
    rate_limited_bl.Cancel();
    blackboard_registered = false;
  }

  DeinitialiseAirspaceBuffer();

  BufferWindow::OnDestroy();
}

void
MapPreviewWindow::OnResize(PixelSize new_size) noexcept
{
  BufferWindow::OnResize(new_size);

  ResizePreviewProjection(new_size);
}

void
MapPreviewWindow::OnPaint(Canvas &canvas) noexcept
{
  BufferWindow::OnPaint(canvas);

  PixelRect rc = GetClientRect();
  if (rc.IsEmpty())
    return;

  /* Inset so the stroke stays inside our clip rect; otherwise bottom/right
     edges are often clipped when nested under #TwoWidgets / #SubCanvas. */
  rc.Grow(-1);
  if (rc.IsEmpty())
    return;

#ifndef USE_GDI
  canvas.DrawOutlineRectangle(rc, COLOR_BLACK);
#else
  canvas.SelectHollowBrush();
  canvas.Select(Pen(Layout::ScalePenWidth(1), COLOR_BLACK));
  canvas.DrawRectangle(rc);
#endif
}

void
MapPreviewWindow::OnPaintBuffer(Canvas &canvas) noexcept
{
#ifdef ENABLE_OPENGL
  const GLCanvasScissor scissor(canvas);
#endif

  LinkDataSources();
  UpdateProjection();

  PaintTerrainAndTopography(canvas);
  PaintAirspace(canvas, true);

  PaintAirspaceMovingMap(canvas);

  PaintOverlays(canvas);
}
