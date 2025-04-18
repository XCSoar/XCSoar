// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
