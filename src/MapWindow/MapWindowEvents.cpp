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

#include "MapWindow.hpp"

#ifndef ENABLE_OPENGL
#include "Screen/WindowCanvas.hpp"
#endif

#include "Weather/Features.hpp"

void
MapWindow::OnResize(PixelSize new_size)
{
  DoubleBufferWindow::OnResize(new_size);

#ifndef ENABLE_OPENGL
  ++ui_generation;

  // We only grow() the buffer here because resizing it everytime has
  // a huge negative effect on the heap fragmentation
  buffer_canvas.Grow(new_size);
#endif

  visible_projection.SetScreenSize(new_size);
  visible_projection.UpdateScreenBounds();
}

void
MapWindow::OnCreate()
{
  DoubleBufferWindow::OnCreate();

#ifndef ENABLE_OPENGL
  WindowCanvas canvas(*this);
  buffer_canvas.Create(canvas);
#endif

  // initialize other systems
  const PixelSize size = GetSize();
  visible_projection.SetScreenSize(size);
  visible_projection.SetMapScale(5000);
  visible_projection.SetScreenOrigin(size.cx / 2, size.cy / 2);
  visible_projection.UpdateScreenBounds();

#ifndef ENABLE_OPENGL
  buffer_projection = visible_projection;
#endif
}

void
MapWindow::OnDestroy()
{
#ifdef HAVE_NOAA
  SetNOAAStore(nullptr);
#endif
  airspace_renderer.Clear();
  SetWaypoints(nullptr);
  SetTopography(nullptr);
  SetTerrain(nullptr);
  SetRasp(nullptr);

#ifndef ENABLE_OPENGL
  buffer_canvas.Destroy();
#endif

  DoubleBufferWindow::OnDestroy();
}

void
MapWindow::OnPaint(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  DoubleBufferWindow::OnPaint(canvas);
#else /* !ENABLE_OPENGL */
  if (buffer_generation == ui_generation)
    DoubleBufferWindow::OnPaint(canvas);
  else if (scale_buffer > 0 && visible_projection.IsValid() &&
           buffer_projection.IsValid()) {
    /* while zooming/panning, project the current buffer into the
       Canvas */

    --scale_buffer;

    /* do the projection */

    const int buffer_width = buffer_projection.GetScreenWidth();
    const int buffer_height = buffer_projection.GetScreenHeight();

    const auto top_left =
      visible_projection.GeoToScreen(buffer_projection.ScreenToGeo(0, 0));
    auto bottom_right =
      visible_projection.GeoToScreen(buffer_projection.ScreenToGeo(buffer_width,
                                                                   buffer_height));

    /* compensate for rounding errors in destination area */

    if (abs(buffer_width - (bottom_right.x - top_left.x)) < 5)
      bottom_right.x = top_left.x + buffer_width;

    if (abs(buffer_height - (bottom_right.y - top_left.y)) < 5)
      bottom_right.y = top_left.y + buffer_height;

    if (top_left.x > bottom_right.x || top_left.y > bottom_right.y) {
      /* paranoid sanity check */
      canvas.ClearWhite();
      return;
    }

    /* clear the areas around the buffer */

    canvas.SelectNullPen();
    canvas.SelectWhiteBrush();

    if (top_left.x > 0)
      canvas.Rectangle(0, 0, top_left.x, canvas.GetHeight());

    if (bottom_right.x < (int)canvas.GetWidth())
      canvas.Rectangle(bottom_right.x, 0,
                       canvas.GetWidth(), canvas.GetHeight());

    if (top_left.y > 0)
      canvas.Rectangle(top_left.x, 0, bottom_right.x, top_left.y);

    if (bottom_right.y < (int)canvas.GetHeight())
      canvas.Rectangle(top_left.x, bottom_right.y,
                       bottom_right.x, canvas.GetHeight());

    /* now stretch the buffer into the window Canvas */

    ScopeLock protect(DoubleBufferWindow::mutex);
    const Canvas &src = GetVisibleCanvas();
    canvas.Stretch(top_left.x, top_left.y,
                   bottom_right.x - top_left.x, bottom_right.y - top_left.y,
                   src, 0, 0, buffer_width, buffer_height);
  } else
    /* the UI has changed since the last DrawThread iteration has
       started: the buffer has invalid data, paint a white window
       instead */
    canvas.ClearWhite();
#endif /* !ENABLE_OPENGL */
}
