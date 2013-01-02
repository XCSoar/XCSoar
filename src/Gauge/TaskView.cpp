/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Gauge/TaskView.hpp"
#include "Screen/Canvas.hpp"
#include "Projection/ChartProjection.hpp"
#include "Renderer/BackgroundRenderer.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "Renderer/OZRenderer.hpp"
#include "Renderer/TaskPointRenderer.hpp"
#include "Renderer/TaskRenderer.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "MapSettings.hpp"

#ifndef ENABLE_OPENGL
#include "Screen/BufferCanvas.hpp"
#else
#include "Screen/OpenGL/Scope.hpp"
#endif

static void
PaintTask(Canvas &canvas, const WindowProjection &projection,
          const OrderedTask &task,
          bool location_available, const GeoPoint &location,
          const MapSettings &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const RasterTerrain *terrain, const Airspaces *airspaces)
{
  BackgroundRenderer background;
  background.SetTerrain(terrain);
  background.Draw(canvas, projection, settings_map.terrain);

  if (airspaces != NULL) {
    AirspaceRenderer airspace_renderer(airspace_look);
    airspace_renderer.SetAirspaces(airspaces);

#ifndef ENABLE_OPENGL
    BufferCanvas buffer_canvas, stencil_canvas;
    buffer_canvas.set(canvas);
    stencil_canvas.set(canvas);
#endif

    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           buffer_canvas, stencil_canvas,
#endif
                           projection,
                           settings_map.airspace);
  }

#ifdef ENABLE_OPENGL
  /* desaturate the map background, to focus on the task */
  canvas.FadeToWhite(0xc0);
#endif

  OZRenderer ozv(task_look, airspace_look, settings_map.airspace);
  TaskPointRenderer tpv(canvas, projection, task_look,
                        task.GetTaskProjection(),
                        ozv, false, TaskPointRenderer::NONE,
                        location_available, location);
  TaskRenderer dv(tpv, projection.GetScreenBounds());
  dv.Draw(task);
}

void
PaintTask(Canvas &canvas, const PixelRect &rc, const OrderedTask &task,
          bool location_available, const GeoPoint &location,
          const MapSettings &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const RasterTerrain *terrain, const Airspaces *airspaces)
{
  /* TODO: check location_available in ChartProjection */
  ChartProjection projection(rc, task, location);
  PaintTask(canvas, projection, task, location_available, location,
            settings_map,
            task_look, airspace_look, terrain, airspaces);
}

void
PaintTaskPoint(Canvas &canvas, const PixelRect &rc,
               const OrderedTask &task, const OrderedTaskPoint &point,
               bool location_available, const GeoPoint &location,
               const MapSettings &settings_map,
               const TaskLook &task_look,
               const AirspaceLook &airspace_look,
               const RasterTerrain *terrain, const Airspaces *airspaces)
{
  /* TODO: check location_available in ChartProjection */
  ChartProjection projection(rc, point, point.GetLocation());
  PaintTask(canvas, projection, task, location_available, location,
            settings_map,
            task_look, airspace_look, terrain, airspaces);
}
