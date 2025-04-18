// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TargetMapWindow.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "Renderer/TaskRenderer.hpp"
#include "Renderer/TaskPointRenderer.hpp"
#include "Renderer/OZRenderer.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/MapScaleRenderer.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Interface.hpp"
#include "Computer/GlideComputer.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/ObservationZones/ObservationZonePoint.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#else
#include "ui/canvas/WindowCanvas.hpp"
#endif

static const ComputerSettings &
GetComputerSettings() noexcept
{
  return CommonInterface::GetComputerSettings();
}

static const MapSettings &
GetMapSettings() noexcept
{
  return CommonInterface::GetMapSettings();
}

static const MoreData &
Basic() noexcept
{
  return CommonInterface::Basic();
}

static const DerivedInfo &
Calculated() noexcept
{
  return CommonInterface::Calculated();
}

/**
 * Constructor of the MapWindow class
 */
TargetMapWindow::TargetMapWindow(const WaypointLook &waypoint_look,
                                 const AirspaceLook &_airspace_look,
                                 const TrailLook &_trail_look,
                                 const TaskLook &_task_look,
                                 const AircraftLook &_aircraft_look,
                                 const TopographyLook &_topography_look,
                                 const OverlayLook &_overlay_look) noexcept
  :task_look(_task_look),
   aircraft_look(_aircraft_look),
   topography_look(_topography_look),
   overlay_look(_overlay_look),
   airspace_renderer(_airspace_look),
   way_point_renderer(nullptr, waypoint_look),
   trail_renderer(_trail_look)
{
}

TargetMapWindow::~TargetMapWindow() noexcept
{
  Destroy();

  delete topography_renderer;
}

void
TargetMapWindow::Create(ContainerWindow &parent, PixelRect rc,
                        WindowStyle style) noexcept
{
  projection.SetScale(0.01);

  BufferWindow::Create(parent, rc, style);
}

inline void
TargetMapWindow::RenderTerrain(Canvas &canvas) noexcept
{
  background.SetShadingAngle(projection, GetMapSettings().terrain,
                             Calculated());
  background.Draw(canvas, projection, GetMapSettings().terrain);
}

inline void
TargetMapWindow::RenderTopography(Canvas &canvas) noexcept
{
  if (topography_renderer != nullptr && GetMapSettings().topography_enabled)
    topography_renderer->Draw(canvas, projection);
}

inline void
TargetMapWindow::RenderTopographyLabels(Canvas &canvas) noexcept
{
  if (topography_renderer != nullptr && GetMapSettings().topography_enabled)
    topography_renderer->DrawLabels(canvas, projection, label_block);
}

inline void
TargetMapWindow::RenderAirspace(Canvas &canvas) noexcept
{
  if (GetMapSettings().airspace.enable)
    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           buffer_canvas,
#endif
                           projection,
                           Basic(), Calculated(),
                           GetComputerSettings().airspace,
                           GetMapSettings().airspace);
}

inline void
TargetMapWindow::DrawTask(Canvas &canvas) noexcept
{
  if (task == nullptr)
    return;

  ProtectedTaskManager::Lease task_manager(*task);
  const AbstractTask *task = task_manager->GetActiveTask();
  if (task && !IsError(task->CheckTask())) {
    OZRenderer ozv(task_look, airspace_renderer.GetLook(),
                   GetMapSettings().airspace);
    TaskPointRenderer tpv(canvas, projection, task_look,
                          /* we're accessing the OrderedTask here,
                             which may be invalid at this point, but it
                             will be used only if active, so it's ok */
                          task_manager->GetOrderedTask().GetTaskProjection(),
                          ozv, false,
                          TaskPointRenderer::TargetVisibility::ALL,
                          Basic().GetLocationOrInvalid());
    tpv.SetTaskFinished(Calculated().task_stats.task_finished);
    TaskRenderer dv(tpv, projection.GetScreenBounds());
    dv.Draw(*task);
  }
}

inline void
TargetMapWindow::DrawWaypoints(Canvas &canvas) noexcept
{
  const MapSettings &settings_map = GetMapSettings();
  WaypointRendererSettings settings = settings_map.waypoint;
  settings.display_text_type = WaypointRendererSettings::DisplayTextType::NAME;

  way_point_renderer.Render(canvas, label_block,
                            projection, settings,
                            GetComputerSettings().polar,
                            GetComputerSettings().task,
                            Basic(), Calculated(),
                            task, nullptr);
}

inline void
TargetMapWindow::RenderTrail(Canvas &canvas) noexcept
{
  if (glide_computer == nullptr)
    return;

  const auto min_time = std::max(Basic().time - std::chrono::minutes{10},
                                 TimeStamp{});
  trail_renderer.Draw(canvas, glide_computer->GetTraceComputer(),
                      projection, min_time);
}

void
TargetMapWindow::OnPaintBuffer(Canvas &canvas) noexcept
{
#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  // Calculate screen position of the aircraft
  const auto aircraft_pos = projection.GeoToScreen(Basic().location);

  // reset label over-write preventer
  label_block.reset();

  // Render terrain, groundline and topography
  RenderTerrain(canvas);
  RenderTopography(canvas);

  // Render airspace
  RenderAirspace(canvas);

#ifdef ENABLE_OPENGL
  /* desaturate the map background, to focus on the task */
  canvas.FadeToWhite(0x80);
#endif

  // Render task, waypoints
  DrawTask(canvas);
  DrawWaypoints(canvas);

  // Render the snail trail
  RenderTrail(canvas);

  // Render topography on top of airspace, to keep the text readable
  RenderTopographyLabels(canvas);

  // Finally, draw you!
  if (Basic().alive)
    AircraftRenderer::Draw(canvas, GetMapSettings(), aircraft_look,
                           Basic().attitude.heading - projection.GetScreenAngle(),
                           aircraft_pos);

  RenderMapScale(canvas, projection, GetClientRect(), overlay_look);
}

void
TargetMapWindow::OnPaint(Canvas &canvas) noexcept
{
  BufferWindow::OnPaint(canvas);

  switch (drag_mode) {
  case DRAG_NONE:
    break;

  case DRAG_TARGET_OUTSIDE:
  case DRAG_TARGET:
  case DRAG_OZ:
    TargetPaintDrag(canvas, drag_last);
    break;
  }
}

void
TargetMapWindow::SetTerrain(RasterTerrain *terrain) noexcept
{
  background.SetTerrain(terrain);
}

void
TargetMapWindow::SetTopograpgy(TopographyStore *topography) noexcept
{
  delete topography_renderer;
  topography_renderer = topography != nullptr
    ? new TopographyRenderer(*topography, topography_look)
    : nullptr;
}

[[gnu::pure]]
static double
GetRadius(const ObservationZonePoint &oz) noexcept
{
  switch (oz.GetShape()) {
  case ObservationZone::Shape::LINE:
  case ObservationZone::Shape::MAT_CYLINDER:
  case ObservationZone::Shape::CYLINDER:
  case ObservationZone::Shape::SECTOR:
  case ObservationZone::Shape::FAI_SECTOR:
  case ObservationZone::Shape::CUSTOM_KEYHOLE:
  case ObservationZone::Shape::DAEC_KEYHOLE:
  case ObservationZone::Shape::BGAFIXEDCOURSE:
  case ObservationZone::Shape::BGAENHANCEDOPTION:
  case ObservationZone::Shape::BGA_START:
  case ObservationZone::Shape::ANNULAR_SECTOR:
  case ObservationZone::Shape::SYMMETRIC_QUADRANT:
    const CylinderZone &cz = (const CylinderZone &)oz;
    return cz.GetRadius();
  }

  return 1;
}

[[gnu::pure]]
static double
GetRadius(const OrderedTaskPoint &tp) noexcept
{
  return GetRadius(tp.GetObservationZone());
}

void
TargetMapWindow::SetTarget(unsigned index) noexcept
{
  GeoPoint location;
  double radius;

  {
    ProtectedTaskManager::Lease lease(*task);
    const OrderedTask &o_task = lease->GetOrderedTask();
    if (!o_task.IsValidIndex(index))
      return;

    const OrderedTaskPoint &tp = o_task.GetTaskPoint(index);
    location = tp.GetLocation();
    radius = std::max(GetRadius(tp) * 1.3, 2000.);
  }

  projection.SetGeoLocation(location);
  projection.SetScaleFromRadius(radius);
  projection.SetScreenAngle(Angle::Zero());
  projection.UpdateScreenBounds();

  target_index = index;

  Invalidate();
}

void
TargetMapWindow::OnResize(PixelSize new_size) noexcept
{
  BufferWindow::OnResize(new_size);

#ifndef ENABLE_OPENGL
  buffer_canvas.Grow(new_size);
#endif

  projection.SetScreenSize(new_size);
  projection.SetScreenOrigin(PixelRect{new_size}.GetCenter());
  projection.UpdateScreenBounds();
}

void
TargetMapWindow::OnCreate()
{
  BufferWindow::OnCreate();

  drag_mode = DRAG_NONE;

#ifndef ENABLE_OPENGL
  WindowCanvas canvas(*this);
  buffer_canvas.Create(canvas);
#endif
}

void
TargetMapWindow::OnDestroy() noexcept
{
  SetTerrain(nullptr);
  SetTopograpgy(nullptr);
  SetAirspaces(nullptr);
  SetWaypoints(nullptr);

#ifndef ENABLE_OPENGL
  buffer_canvas.Destroy();
#endif

  BufferWindow::OnDestroy();
}
