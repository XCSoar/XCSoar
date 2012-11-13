/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
DoubleBufferWindow::OnCreate()
{
  PaintWindow::OnCreate();

  dirty = true;
}

void
DoubleBufferWindow::OnDestroy()
{
  buffer.Destroy();
  PaintWindow::OnDestroy();
}

void
DoubleBufferWindow::OnResize(UPixelScalar width, UPixelScalar height)
{
  PaintWindow::OnResize(width, height);
  buffer.Destroy();
  Invalidate();
}

#else /* !OpenGL */

#include "Screen/WindowCanvas.hpp"

void
DoubleBufferWindow::OnCreate()
{
  PaintWindow::OnCreate();

  WindowCanvas a_canvas(*this);
  buffers[0].Create(a_canvas);
  buffers[1].Create(a_canvas);
}

void
DoubleBufferWindow::OnDestroy()
{
  PaintWindow::OnDestroy();

  buffers[0].Destroy();
  buffers[1].Destroy();
}

void
DoubleBufferWindow::flip()
{
  /* enable the drawing buffer */
  mutex.Lock();
  current ^= 1;
  mutex.Unlock();

  /* commit the finished buffer to the screen (asynchronously) */
  Invalidate();

  /* grow the current buffer, just in case the window has been
     resized */
  buffers[current].Grow(GetWidth(), GetHeight());
}

#endif

void
DoubleBufferWindow::OnPaint(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  if (!buffer.IsDefined()) {
    buffer.Create(canvas, canvas.GetWidth(), canvas.GetHeight());
    dirty = true;
  }

  if (dirty) {
    dirty = false;
    buffer.Begin(canvas);
    OnPaintBuffer(buffer);
    buffer.Commit(canvas);
  } else
    buffer.CopyTo(canvas);

#else
  ScopeLock protect(mutex);
  canvas.copy(get_visible_canvas());
#endif
}
