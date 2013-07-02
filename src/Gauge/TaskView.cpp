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
#include "Renderer/FAITriangleAreaRenderer.hpp"
#include "Look/AirspaceLook.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Look/TaskLook.hpp"
#include "MapSettings.hpp"

#ifndef ENABLE_OPENGL
#include "Screen/BufferCanvas.hpp"
#else
#include "Screen/OpenGL/Scope.hpp"
#endif

gcc_pure
static bool
IsFAITriangleApplicable(TaskFactoryType factory)
{
  switch (factory) {
  case TaskFactoryType::FAI_GENERAL:
  case TaskFactoryType::FAI_TRIANGLE:
  case TaskFactoryType::FAI_GOAL:
  case TaskFactoryType::RACING:
  case TaskFactoryType::MIXED:
  case TaskFactoryType::TOURING:
    return true;

  case TaskFactoryType::FAI_OR:
  case TaskFactoryType::MAT:
  case TaskFactoryType::AAT:
    return false;

  case TaskFactoryType::COUNT:
    gcc_unreachable();
  }

  gcc_unreachable();
}

gcc_pure
static bool
IsFAITriangleApplicable(const OrderedTask &task)
{
  return IsFAITriangleApplicable(task.GetFactoryType()) &&
    !task.HasOptionalStarts() &&
    // TODO: allow start on first leg
    task.TaskSize() >= 2 && task.TaskSize() <= 4;
}

static void
RenderFAISectors(Canvas &canvas, const WindowProjection &projection,
                 const OrderedTask &task)
{
  const unsigned size = task.TaskSize();
  const unsigned end = size - 1;

  for (unsigned i = 0; i != end; ++i)
    RenderFAISector(canvas, projection,
                    task.GetPoint(i).GetLocation(),
                    task.GetPoint(i + 1).GetLocation(),
                    true);

  for (unsigned i = 0; i != end; ++i)
    RenderFAISector(canvas, projection,
                    task.GetPoint(i).GetLocation(),
                    task.GetPoint(i + 1).GetLocation(),
                    false);
}

void
PaintTask(Canvas &canvas, const WindowProjection &projection,
          const OrderedTask &task,
          bool location_available, const GeoPoint &location,
          const MapSettings &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const RasterTerrain *terrain, const Airspaces *airspaces,
          bool fai_sectors,
          int highlight_index)
{
  BackgroundRenderer background;
  background.SetTerrain(terrain);
  background.Draw(canvas, projection, settings_map.terrain);

  if (airspaces != NULL) {
    AirspaceRenderer airspace_renderer(airspace_look);
    airspace_renderer.SetAirspaces(airspaces);

#ifndef ENABLE_OPENGL
    BufferCanvas buffer_canvas, stencil_canvas;
    buffer_canvas.Create(canvas);
    stencil_canvas.Create(canvas);
#endif

    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           buffer_canvas, stencil_canvas,
#endif
                           projection, settings_map.airspace);
  }

#ifdef ENABLE_OPENGL
  /* desaturate the map background, to focus on the task */
  canvas.FadeToWhite(0xc0);
#endif

  if (fai_sectors && IsFAITriangleApplicable(task)) {
    static constexpr Color fill_color = COLOR_YELLOW;
#if defined(ENABLE_OPENGL) || defined(USE_MEMORY_CANVAS)
#ifdef ENABLE_OPENGL
    const GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

    canvas.Select(Brush(fill_color.WithAlpha(40)));
    canvas.Select(Pen(1, COLOR_BLACK.WithAlpha(80)));
    RenderFAISectors(canvas, projection, task);
#else
    BufferCanvas buffer_canvas;
    buffer_canvas.Create(canvas);
    buffer_canvas.ClearWhite();
#ifdef HAVE_HATCHED_BRUSH
    buffer_canvas.Select(airspace_look.brushes[3]);
    buffer_canvas.SetTextColor(fill_color);
    buffer_canvas.SetBackgroundColor(COLOR_WHITE);
#else
    buffer_canvas.Select(Brush(fill_color));
#endif
    buffer_canvas.SelectNullPen();
    RenderFAISectors(buffer_canvas, projection, task);
    canvas.CopyAnd(buffer_canvas);

    canvas.SelectHollowBrush();
    canvas.SelectBlackPen();
    RenderFAISectors(canvas, projection, task);
#endif
  }

  OZRenderer ozv(task_look, airspace_look, settings_map.airspace);
  TaskPointRenderer tpv(canvas, projection, task_look,
                        task.GetTaskProjection(),
                        ozv, false, TaskPointRenderer::NONE,
                        location_available, location);
  TaskRenderer dv(tpv, projection.GetScreenBounds());
  dv.Draw(task);

  // highlight a task point
  if (highlight_index >= 0 && highlight_index < (int) task.TaskSize()) {
    /* TODO: clumsy way of highlighting. maybe it should be done by
     *       painting the task point with a different pen and brush,
     *       e.g. red, 4px wide
     */
    RasterPoint pt = projection.GeoToScreen(task.GetPoint(highlight_index).
                                            GetLocation());
    canvas.Select(task_look.highlight_pen);
    canvas.DrawLine(pt.x - 7, pt.y - 7, pt.x + 7, pt.y + 7);
    canvas.DrawLine(pt.x + 7, pt.y - 7, pt.x - 7, pt.y + 7);
  }
}

void
PaintTask(Canvas &canvas, const PixelRect &rc, const OrderedTask &task,
          bool location_available, const GeoPoint &location,
          const MapSettings &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const RasterTerrain *terrain, const Airspaces *airspaces,
          bool fai_sectors,
          int highlight_index)
{
  /* TODO: check location_available in ChartProjection */
  ChartProjection projection(rc, task, location);
  PaintTask(canvas, projection, task, location_available, location,
            settings_map,
            task_look, airspace_look, terrain, airspaces,
            fai_sectors, highlight_index);
}

void
PaintTaskPoint(Canvas &canvas, const PixelRect &rc,
               const OrderedTask &task, const OrderedTaskPoint &point,
               bool location_available, const GeoPoint &location,
               const MapSettings &settings_map, const TaskLook &task_look,
               const AirspaceLook &airspace_look,
               const RasterTerrain *terrain, const Airspaces *airspaces,
               int highlight_index)
{
  /* TODO: check location_available in ChartProjection */
  ChartProjection projection(rc, point, point.GetLocation());
  PaintTask(canvas, projection, task, location_available, location,
            settings_map,
            task_look, airspace_look, terrain, airspaces,
            false, highlight_index);
}
