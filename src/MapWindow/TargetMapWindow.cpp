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

#include "TargetMapWindow.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Look/TaskLook.hpp"
#include "Renderer/TaskRenderer.hpp"
#include "Renderer/RenderTaskPoint.hpp"
#include "Renderer/OZRenderer.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/TrailRenderer.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Math/Earth.hpp"
#include "Units/Units.hpp"
#include "Interface.hpp"
#include "Computer/GlideComputer.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#else
#include "Screen/WindowCanvas.hpp"
#endif

#include <tchar.h>

static const SETTINGS_COMPUTER &
SettingsComputer()
{
  return CommonInterface::SettingsComputer();
}

static const SETTINGS_MAP &
SettingsMap()
{
  return CommonInterface::SettingsMap();
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
                                 const TaskLook &_task_look,
                                 const AircraftLook &_aircraft_look)
  :task_look(_task_look),
   aircraft_look(_aircraft_look),
   topography_renderer(NULL),
   airspace_renderer(_airspace_look),
   way_point_renderer(NULL, waypoint_look),
   task(NULL)
{
}

TargetMapWindow::~TargetMapWindow()
{
  delete topography_renderer;
}

void
TargetMapWindow::set(ContainerWindow &parent, int left, int top,
                     unsigned width, unsigned height, WindowStyle style)
{
  projection.SetFreeMapScale(fixed_int_constant(5000));

  BufferWindow::set(parent, left, top, width, height, style);
}

void
TargetMapWindow::RenderTerrain(Canvas &canvas)
{
  if (SettingsMap().terrain.slope_shading == sstWind)
    background.sun_from_wind(projection, Calculated().wind);
  else
    background.set_sun_angle(projection,
                             (Basic().location_available &&
                              SettingsMap().terrain.slope_shading == sstSun) ?
                             Calculated().sun_azimuth :
                             Angle::degrees(fixed(-45.0)));

  background.Draw(canvas, projection, SettingsMap().terrain);
}

void
TargetMapWindow::RenderTopography(Canvas &canvas)
{
  if (topography_renderer != NULL && SettingsMap().EnableTopography)
    topography_renderer->Draw(canvas, projection);
}

void
TargetMapWindow::RenderTopographyLabels(Canvas &canvas)
{
  if (topography_renderer != NULL && SettingsMap().EnableTopography)
    topography_renderer->DrawLabels(canvas, projection, label_block,
                                    SettingsMap());
}

void
TargetMapWindow::RenderAirspace(Canvas &canvas)
{
  if (SettingsMap().airspace.enable)
    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           buffer_canvas, stencil_canvas,
#endif
                           projection,
                           Basic(), Calculated(),
                           SettingsComputer(), SettingsMap());
}

class RenderTaskPointMap : public RenderTaskPoint {
public:
  RenderTaskPointMap(Canvas &_canvas,
                     const WindowProjection &_projection,
                     const TaskLook &task_look,
                     const TaskProjection &_task_projection,
                     OZRenderer &_ozv,
                     const bool draw_bearing,
                     TargetVisibility _target_visibility,
                     const GeoPoint &location):
    RenderTaskPoint(_canvas, _projection,
                    task_look, _task_projection,
                    _ozv, draw_bearing, _target_visibility, location)
    {};

protected:
  void
  draw_target(const TaskPoint &tp)
  {
    if (!DoDrawTarget(tp))
      return;

    RasterPoint sc;
    if (m_proj.GeoToScreenIfVisible(tp.GetLocationRemaining(), sc))
      task_look.target_icon.draw(canvas, sc.x, sc.y);
  }
};

void
TargetMapWindow::DrawTask(Canvas &canvas)
{
  if (task == NULL)
    return;

  ProtectedTaskManager::Lease task_manager(*task);
  const AbstractTask *task = task_manager->get_active_task();
  if (task && task->check_task()) {

    OZRenderer ozv(task_look, airspace_renderer.GetLook(),
                              SettingsMap().airspace);
    RenderTaskPointMap tpv(canvas,
                           projection,
                           task_look,
                           /* we're accessing the OrderedTask here,
                              which may be invalid at this point, but it
                              will be used only if active, so it's ok */
                           task_manager->get_ordered_task().get_task_projection(),
                           ozv, false, RenderTaskPoint::ALL,
                           Basic().location);
    TaskRenderer dv(tpv, projection.GetScreenBounds());
    dv.Draw(*task);
  }
}

void
TargetMapWindow::DrawWaypoints(Canvas &canvas)
{
  const SETTINGS_MAP &settings_map = SettingsMap();
  WaypointRendererSettings settings = settings_map.waypoint;
  settings.display_text_type = DISPLAYNAME;

  way_point_renderer.render(canvas, label_block,
                            projection, settings,
                            SettingsComputer().task,
                            task, NULL);
}

void
TargetMapWindow::RenderTrail(Canvas &canvas) const
{
  if (glide_computer == NULL)
    return;

  unsigned min_time = max(0, (int)Basic().time - 600);
  TrailRenderer::Draw(canvas, *glide_computer, projection, min_time);
}

void
TargetMapWindow::on_paint_buffer(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  // Calculate screen position of the aircraft
  const RasterPoint aircraft_pos = projection.GeoToScreen(Basic().location);

  // reset label over-write preventer
  label_block.reset();

  // Render terrain, groundline and topography
  RenderTerrain(canvas);
  RenderTopography(canvas);

  // Render airspace
  RenderAirspace(canvas);

  // Render task, waypoints
  DrawTask(canvas);
  DrawWaypoints(canvas);

  // Render the snail trail
  RenderTrail(canvas);

  // Render topography on top of airspace, to keep the text readable
  RenderTopographyLabels(canvas);

  // Finally, draw you!
  if (Basic().connected)
    AircraftRenderer::Draw(canvas, SettingsMap(), aircraft_look,
                           Calculated().heading - projection.GetScreenAngle(),
                           aircraft_pos);
}

void
TargetMapWindow::on_paint(Canvas &canvas)
{
  BufferWindow::on_paint(canvas);

  if (drag_mode == DRAG_TARGET)
    TargetPaintDrag(canvas, drag_last);
}

void
TargetMapWindow::SetTerrain(RasterTerrain *terrain)
{
  background.set_terrain(terrain);
}

void
TargetMapWindow::SetTopograpgy(TopographyStore *topography)
{
  delete topography_renderer;
  topography_renderer = topography != NULL
    ? new TopographyRenderer(*topography)
    : NULL;
}

static fixed
GetRadius(const ObservationZonePoint &oz)
{
  switch (oz.shape) {
  case ObservationZonePoint::LINE:
  case ObservationZonePoint::CYLINDER:
  case ObservationZonePoint::SECTOR:
  case ObservationZonePoint::FAI_SECTOR:
  case ObservationZonePoint::KEYHOLE:
  case ObservationZonePoint::BGAFIXEDCOURSE:
  case ObservationZonePoint::BGAENHANCEDOPTION:
  case ObservationZonePoint::BGA_START:
  case ObservationZonePoint::ANNULAR_SECTOR:
    CylinderZone &cz = (CylinderZone &)oz;
    return cz.getRadius();
  }

  return fixed_one;
}

static fixed
GetRadius(const OrderedTaskPoint &tp)
{
  return GetRadius(*tp.get_oz());
}

void
TargetMapWindow::SetTarget(unsigned index)
{
  GeoPoint location;
  fixed radius;

  {
    ProtectedTaskManager::Lease lease(*task);
    const OrderedTask &o_task = lease->get_ordered_task();
    const OrderedTaskPoint *tp = o_task.getTaskPoint(index);
    if (tp == NULL)
      return;

    location = tp->GetLocation();
    radius = std::max(GetRadius(*tp) * fixed(1.3), fixed(2000));
  }

  projection.SetGeoLocation(location);
  projection.SetScale(fixed(projection.GetScreenDistance()) / (radius * 2));
  projection.SetScreenAngle(Angle::zero());
  projection.UpdateScreenBounds();

  target_index = index;

  invalidate();
}

bool
TargetMapWindow::on_resize(unsigned width, unsigned height)
{
  BufferWindow::on_resize(width, height);

#ifndef ENABLE_OPENGL
  buffer_canvas.grow(width, height);
  stencil_canvas.grow(width, height);
#endif

  projection.SetScreenSize(width, height);
  projection.SetScreenOrigin(width / 2, height / 2);
  projection.UpdateScreenBounds();

  return true;
}

bool
TargetMapWindow::on_create()
{
  if (!BufferWindow::on_create())
    return false;

  drag_mode = DRAG_NONE;

#ifndef ENABLE_OPENGL
  WindowCanvas canvas(*this);
  buffer_canvas.set(canvas);
  stencil_canvas.set(canvas);
#endif
  return true;
}

bool
TargetMapWindow::on_destroy()
{
  SetTerrain(NULL);
  SetTopograpgy(NULL);
  SetAirspaces(NULL);
  SetWaypoints(NULL);

#ifndef ENABLE_OPENGL
  buffer_canvas.reset();
  stencil_canvas.reset();
#endif

  BufferWindow::on_destroy();
  return true;
}
