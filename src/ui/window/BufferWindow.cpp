// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BufferWindow.hpp"

#ifndef ENABLE_OPENGL
#include "ui/canvas/WindowCanvas.hpp"
#endif

void
BufferWindow::OnResize(PixelSize new_size) noexcept
{
  if (buffer.IsDefined()) {
#ifdef ENABLE_OPENGL
    buffer.Destroy();
#else
    buffer.Resize(new_size);
    Invalidate();
#endif
  }

  PaintWindow::OnResize(new_size);
}

void
BufferWindow::OnPaint(Canvas &canvas) noexcept
{
  if (!buffer.IsDefined()) {
    buffer.Create(canvas);
    dirty = true;
  }

#ifdef ENABLE_OPENGL
  if (dirty) {
    dirty = false;
    buffer.Begin(canvas);
    OnPaintBuffer(buffer);
    buffer.Commit(canvas);
  } else
    buffer.CopyTo(canvas);

#else

  if (dirty) {
    dirty = false;
    OnPaintBuffer(buffer);
  }

  canvas.Copy(buffer);
#endif
}
