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

#include "Screen/BufferWindow.hpp"

#ifndef ENABLE_OPENGL

#include "Screen/WindowCanvas.hpp"

void
BufferWindow::OnCreate()
{
  PaintWindow::OnCreate();

  dirty = true;

  WindowCanvas a_canvas(*this);
  buffer.Create(a_canvas);
}

void
BufferWindow::OnDestroy()
{
  PaintWindow::OnDestroy();

  buffer.Destroy();
}

void
BufferWindow::OnResize(UPixelScalar width, UPixelScalar height)
{
  buffer.Resize(width, height);
  PaintWindow::OnResize(width, height);
  Invalidate();
}

#endif

void
BufferWindow::OnPaint(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  /* paint directly on OpenGL */
  OnPaintBuffer(canvas);
#else
  if (dirty) {
    dirty = false;
    OnPaintBuffer(buffer);
  }

  canvas.Copy(buffer);
#endif
}
