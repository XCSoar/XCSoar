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
#include "Screen/OpenGL/Scissor.hpp"
#else
#include "Screen/WindowCanvas.hpp"
#endif

static const ComputerSettings &
GetComputerSettings()
{
  return CommonInterface::GetComputerSettings();
}

static const MapSettings &
GetMapSettings()
{
  return CommonInterface::GetMapSettings();
}

static const MoreData &
Basic()
{
  return CommonInterface::Basic();
}

static const DerivedInfo &
Calculated()
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
                                 const OverlayLook &_overlay_look)
  :task_look(_task_look),
   aircraft_look(_aircraft_look),
   topography_look(_topography_look),
   overlay_look(_overlay_look),
   airspace_renderer(_airspace_look),
   way_point_renderer(nullptr, waypoint_look),
   trail_renderer(_trail_look)
{
}

TargetMapWindow::~TargetMapWindow()
{
  Destroy();

  delete topography_renderer;
}

void
TargetMapWindow::Create(ContainerWindow &parent, PixelRect rc,
                        WindowStyle style)
{
  projection.SetScale(0.01);

  BufferWindow::Create(parent, rc, style);
}

void
TargetMapWindow::RenderTerrain(Canvas &canvas)
{
  background.SetShadingAngle(projection, GetMapSettings().terrain,
                             Calculated());
  background.Draw(canvas, projection, GetMapSettings().terrain);
}

void
TargetMapWindow::RenderTopography(Canvas &canvas)
{
  if (topography_renderer != nullptr && GetMapSettings().topography_enabled)
    topography_renderer->Draw(canvas, projection);
}

void
TargetMapWindow::RenderTopographyLabels(Canvas &canvas)
{
  if (topography_renderer != nullptr && GetMapSettings().topography_enabled)
    topography_renderer->DrawLabels(canvas, projection, label_block);
}

void
TargetMapWindow::RenderAirspace(Canvas &canvas)
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

void
TargetMapWindow::DrawTask(Canvas &canvas)
{
  if (task == nullptr)
    return;

  ProtectedTaskManager::Lease task_manager(*task);
  const AbstractTask *task = task_manager->GetActiveTask();
  if (task && task->CheckTask()) {
    OZRenderer ozv(task_look, airspace_renderer.GetLook(),
                   GetMapSettings().airspace);
    TaskPointRenderer tpv(canvas, projection, task_look,
                          /* we're accessing the OrderedTask here,
                             which may be invalid at this point, but it
                             will be used only if active, so it's ok */
                          task_manager->GetOrderedTask().GetTaskProjection(),
                          ozv, false, TaskPointRenderer::ALL,
                          Basic().location_available
                          ? Basic().location : GeoPoint::Invalid());
    tpv.SetTaskFinished(Calculated().task_stats.task_finished);
    TaskRenderer dv(tpv, projection.GetScreenBounds());
    dv.Draw(*task);
  }
}

void
TargetMapWindow::DrawWaypoints(Canvas &canvas)
{
  const MapSettings &settings_map = GetMapSettings();
  WaypointRendererSettings settings = settings_map.waypoint;
  settings.display_text_type = WaypointRendererSettings::DisplayTextType::NAME;

  way_point_renderer.render(canvas, label_block,
                            projection, settings,
                            GetComputerSettings().polar,
                            GetComputerSettings().task,
                            Basic(), Calculated(),
                            task, nullptr);
}

void
TargetMapWindow::RenderTrail(Canvas &canvas)
{
  if (glide_computer == nullptr)
    return;

  unsigned min_time = std::max(0, (int)Basic().time - 600);
  trail_renderer.Draw(canvas, glide_computer->GetTraceComputer(),
                      projection, min_time);
}

void
TargetMapWindow::OnPaintBuffer(Canvas &canvas)
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
TargetMapWindow::OnPaint(Canvas &canvas)
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
TargetMapWindow::SetTerrain(RasterTerrain *terrain)
{
  background.SetTerrain(terrain);
}

void
TargetMapWindow::SetTopograpgy(TopographyStore *topography)
{
  delete topography_renderer;
  topography_renderer = topography != nullptr
    ? new TopographyRenderer(*topography, topography_look)
    : nullptr;
}

gcc_pure
static double
GetRadius(const ObservationZonePoint &oz)
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

gcc_pure
static double
GetRadius(const OrderedTaskPoint &tp)
{
  return GetRadius(tp.GetObservationZone());
}

void
TargetMapWindow::SetTarget(unsigned index)
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
TargetMapWindow::OnResize(PixelSize new_size)
{
  BufferWindow::OnResize(new_size);

#ifndef ENABLE_OPENGL
  buffer_canvas.Grow(new_size);
#endif

  projection.SetScreenSize(new_size);
  projection.SetScreenOrigin(new_size.cx / 2, new_size.cy / 2);
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
TargetMapWindow::OnDestroy()
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
