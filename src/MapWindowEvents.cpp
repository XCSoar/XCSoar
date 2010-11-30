/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "MapWindow.hpp"

#ifdef ENABLE_OPENGL
#include "Protection.hpp"
#include "DeviceBlackboard.hpp"
#else
#include "Screen/WindowCanvas.hpp"
#endif

bool
MapWindow::on_resize(unsigned width, unsigned height)
{
  DoubleBufferWindow::on_resize(width, height);

#ifndef ENABLE_OPENGL
  ++ui_generation;

  buffer_canvas.grow(width, height);
  stencil_canvas.grow(width, height);
#endif

  visible_projection.SetMapRect(get_client_rect());
  visible_projection.UpdateScreenBounds();

  return true;
}

bool
MapWindow::on_create()
{
  if (!DoubleBufferWindow::on_create())
    return false;

#ifndef ENABLE_OPENGL
  WindowCanvas canvas(*this);
  buffer_canvas.set(canvas);
  stencil_canvas.set(canvas);
#endif
  return true;
}

bool
MapWindow::on_destroy()
{
  set_marks(NULL);
  set_airspaces(NULL, NULL);
  set_way_points(NULL);
  set_topology(NULL);
  set_terrain(NULL);
  set_weather(NULL);

#ifndef ENABLE_OPENGL
  buffer_canvas.reset();
  stencil_canvas.reset();
#endif

  DoubleBufferWindow::on_destroy();
  return true;
}

void
MapWindow::on_paint(Canvas &canvas)
{
#ifdef ENABLE_OPENGL

  /* copy device_blackboard to MapWindow */

  mutexBlackboard.Lock();
  ReadBlackboard(device_blackboard.Basic(), device_blackboard.Calculated(),
                 device_blackboard.SettingsComputer(),
                 device_blackboard.SettingsMap());
  mutexBlackboard.Unlock();

  UpdateMapScale();

  /* paint it */

  on_paint_buffer(canvas);

  /* copy MapWindow to device_blackboard */

  mutexBlackboard.Lock();
  device_blackboard.ReadMapProjection(VisibleProjection());
  mutexBlackboard.Unlock();

#else /* !ENABLE_OPENGL */
  if (buffer_generation == ui_generation)
    DoubleBufferWindow::on_paint(canvas);
  else if (scale_buffer > 0) {
    /* while zooming/panning, project the current buffer into the
       Canvas */

    --scale_buffer;

    /* do the projection */

    const RECT buffer_rect = buffer_projection.GetMapRect();
    const RasterPoint top_left =
      visible_projection.GeoToScreen(buffer_projection.ScreenToGeo(buffer_rect.left,
                                                                       buffer_rect.top));
    RasterPoint bottom_right =
      visible_projection.GeoToScreen(buffer_projection.ScreenToGeo(buffer_rect.right,
                                                                       buffer_rect.bottom));

    /* compensate for rounding errors in destination area */

    if (abs((buffer_rect.right - buffer_rect.left) -
            (bottom_right.x - top_left.x)) < 5)
      bottom_right.x = top_left.x + buffer_rect.right - buffer_rect.left;

    if (abs((buffer_rect.bottom - buffer_rect.top) -
            (bottom_right.y - top_left.y)) < 5)
      bottom_right.y = top_left.y + buffer_rect.bottom - buffer_rect.top;

    /* clear the areas around the buffer */

    canvas.null_pen();
    canvas.white_brush();

    if (top_left.x > 0)
      canvas.rectangle(0, 0, top_left.x, canvas.get_height());

    if (bottom_right.x < (int)canvas.get_width())
      canvas.rectangle(bottom_right.x, 0,
                       canvas.get_width(), canvas.get_height());

    if (top_left.y > 0)
      canvas.rectangle(top_left.x, 0, bottom_right.x, top_left.y);

    if (bottom_right.y < (int)canvas.get_height())
      canvas.rectangle(top_left.x, bottom_right.y,
                       bottom_right.x, canvas.get_height());

    /* now stretch the buffer into the window Canvas */

    ScopeLock protect(DoubleBufferWindow::mutex);
    const Canvas &src = get_visible_canvas();
    canvas.stretch(top_left.x, top_left.y,
                   bottom_right.x - top_left.x, bottom_right.y - top_left.y,
                   src, buffer_rect.left, buffer_rect.top,
                   buffer_rect.right - buffer_rect.left,
                   buffer_rect.bottom - buffer_rect.top);
  } else
    /* the UI has changed since the last DrawThread iteration has
       started: the buffer has invalid data, paint a white window
       instead */
    canvas.clear_white();
#endif /* !ENABLE_OPENGL */
}
