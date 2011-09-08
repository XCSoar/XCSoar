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

#include "Screen/BufferWindow.hpp"

#ifndef ENABLE_OPENGL

#include "Screen/WindowCanvas.hpp"

bool
BufferWindow::on_create()
{
  if (!PaintWindow::on_create())
    return false;

  dirty = true;

  WindowCanvas a_canvas(*this);
  buffer.set(a_canvas);
  return true;
}

bool
BufferWindow::on_destroy()
{
  PaintWindow::on_destroy();

  buffer.reset();
  return true;
}

bool
BufferWindow::on_resize(unsigned width, unsigned height)
{
  buffer.resize(width, height);
  PaintWindow::on_resize(width, height);
  invalidate();
  return true;
}

#endif

void
BufferWindow::on_paint(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  /* paint directly on OpenGL */
  on_paint_buffer(canvas);
#else
  if (dirty) {
    dirty = false;
    on_paint_buffer(buffer);
  }

  canvas.copy(buffer);
#endif
}
