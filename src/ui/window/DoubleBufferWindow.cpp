/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "DoubleBufferWindow.hpp"

#ifndef ENABLE_OPENGL

#include "ui/canvas/WindowCanvas.hpp"

void
DoubleBufferWindow::OnCreate()
{
  PaintWindow::OnCreate();

  next_size = GetSize();

  WindowCanvas a_canvas(*this);
  buffers[0].Create(a_canvas);
  buffers[1].Create(a_canvas);
}

void
DoubleBufferWindow::OnDestroy() noexcept
{
  PaintWindow::OnDestroy();

  buffers[0].Destroy();
  buffers[1].Destroy();
}

void
DoubleBufferWindow::OnResize(PixelSize new_size) noexcept
{
  PaintWindow::OnResize(new_size);

  /* store the new size for the next Repaint() call in a thread-safe
     field */
  const std::lock_guard lock{mutex};
  next_size = new_size;
}

void
DoubleBufferWindow::Repaint() noexcept
{
  {
    const std::lock_guard lock{mutex};
    auto &canvas = GetPaintCanvas();

    /* grow the current buffer, just in case the window has been
       resized */
    canvas.Grow(next_size);

    OnPaintBuffer(canvas);

    current ^= 1;
  }

  /* commit the finished buffer to the screen (asynchronously) */
  invalidate_notify.SendNotification();
}

void
DoubleBufferWindow::OnPaint(Canvas &canvas) noexcept
{
  const std::lock_guard lock{mutex};
  canvas.Copy(GetVisibleCanvas());
}

#endif
