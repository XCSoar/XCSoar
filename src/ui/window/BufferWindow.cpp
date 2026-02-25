// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BufferWindow.hpp"

#ifndef ENABLE_OPENGL
#include "ui/canvas/WindowCanvas.hpp"
#endif

void
BufferWindow::OnResize(PixelSize new_size) noexcept
{
#ifdef USE_MEMORY_CANVAS
  // For memory canvas, defer the actual resize to OnPaint
  // to avoid race conditions with concurrent drawing
  if (buffer.IsDefined()) {
    // Store size atomically
    pending_width.store(new_size.width, std::memory_order_relaxed);
    pending_height.store(new_size.height, std::memory_order_relaxed);
    // Set flag with release semantics
    resize_pending.store(true, std::memory_order_release);
    Invalidate();
  }
#else
  if (buffer.IsDefined()) {
#ifdef ENABLE_OPENGL
    buffer.Destroy();
#else
    buffer.Resize(new_size);
    Invalidate();
#endif
  }
#endif

  PaintWindow::OnResize(new_size);
}

void
BufferWindow::OnPaint(Canvas &canvas) noexcept
{
  if (!buffer.IsDefined()) {
    buffer.Create(canvas);
    dirty = true;
  }

#ifdef USE_MEMORY_CANVAS
  // Process any pending resize before painting
  // Use exchange to atomically check and clear the flag
  if (resize_pending.exchange(false, std::memory_order_acquire)) {
    // Read the size atomically
    const PixelSize new_size{
      pending_width.load(std::memory_order_relaxed),
      pending_height.load(std::memory_order_relaxed)
    };
    buffer.Resize(new_size);
    dirty = true;
  }
#endif

#ifdef ENABLE_OPENGL
  /* Disable the cache replay path and always repaint.  This avoids
     reusing stale cached buffers after display orientation changes. */
  dirty = false;
  buffer.Begin(canvas);
  OnPaintBuffer(buffer);
  buffer.Commit(canvas);

#else

  if (dirty) {
    dirty = false;
    OnPaintBuffer(buffer);
  }

  canvas.Copy(buffer);
#endif
}
