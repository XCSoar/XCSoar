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

#include "Screen/DoubleBufferWindow.hpp"

#ifdef ENABLE_OPENGL

void
DoubleBufferWindow::on_create()
{
  PaintWindow::on_create();

  dirty = true;
}

void
DoubleBufferWindow::on_destroy()
{
  buffer.reset();
  PaintWindow::on_destroy();
}

void
DoubleBufferWindow::on_resize(UPixelScalar width, UPixelScalar height)
{
  PaintWindow::on_resize(width, height);
  buffer.reset();
  invalidate();
}

#else /* !OpenGL */

#include "Screen/WindowCanvas.hpp"

void
DoubleBufferWindow::on_create()
{
  PaintWindow::on_create();

  WindowCanvas a_canvas(*this);
  buffers[0].set(a_canvas);
  buffers[1].set(a_canvas);
}

void
DoubleBufferWindow::on_destroy()
{
  PaintWindow::on_destroy();

  buffers[0].reset();
  buffers[1].reset();
}

void
DoubleBufferWindow::flip()
{
  /* enable the drawing buffer */
  mutex.Lock();
  current ^= 1;
  mutex.Unlock();

  /* commit the finished buffer to the screen (asynchronously) */
  invalidate();

  /* grow the current buffer, just in case the window has been
     resized */
  buffers[current].grow(get_width(), get_height());
}

#endif

void
DoubleBufferWindow::on_paint(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  if (!buffer.defined()) {
    buffer.set(canvas, canvas.get_width(), canvas.get_height());
    dirty = true;
  }

  if (dirty) {
    dirty = false;
    buffer.Begin(canvas);
    on_paint_buffer(buffer);
    buffer.Commit(canvas);
  } else
    buffer.CopyTo(canvas);

#else
  ScopeLock protect(mutex);
  canvas.copy(get_visible_canvas());
#endif
}
