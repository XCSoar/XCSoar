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

  // We only grow() the buffer here because resizing it everytime has
  // a huge negative effect on the heap fragmentation
  buffer_canvas.grow(width, height);
  stencil_canvas.grow(width, height);
#endif

  visible_projection.SetScreenSize(width, height);
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
  set_topography(NULL);
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
  on_paint_buffer(canvas);
#else /* !ENABLE_OPENGL */
  if (buffer_generation == ui_generation)
    DoubleBufferWindow::on_paint(canvas);
  else if (scale_buffer > 0) {
    /* while zooming/panning, project the current buffer into the
       Canvas */

    --scale_buffer;

    /* do the projection */

    const unsigned buffer_width = buffer_projection.GetScreenWidth();
    const unsigned buffer_height = buffer_projection.GetScreenHeight();

    const RasterPoint top_left =
      visible_projection.GeoToScreen(buffer_projection.ScreenToGeo(0, 0));
    RasterPoint bottom_right =
      visible_projection.GeoToScreen(buffer_projection.ScreenToGeo(buffer_width,
                                                                   buffer_height));

    /* compensate for rounding errors in destination area */

    if (abs(buffer_width - (bottom_right.x - top_left.x)) < 5)
      bottom_right.x = top_left.x + buffer_width;

    if (abs(buffer_height - (bottom_right.y - top_left.y)) < 5)
      bottom_right.y = top_left.y + buffer_height;

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
                   src, 0, 0, buffer_width, buffer_height);
  } else
    /* the UI has changed since the last DrawThread iteration has
       started: the buffer has invalid data, paint a white window
       instead */
    canvas.clear_white();
#endif /* !ENABLE_OPENGL */
}
