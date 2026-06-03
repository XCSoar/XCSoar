// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BufferWindow.hpp"
#include "TopWindow.hpp"

#ifndef ENABLE_OPENGL
#include "ui/canvas/WindowCanvas.hpp"
#endif

void
BufferWindow::OnResize(PixelSize new_size) noexcept
{
#if defined(USE_MEMORY_CANVAS) || defined(ENABLE_OPENGL)
  if (buffer.IsDefined()) {
    pending_width.store(new_size.width, std::memory_order_relaxed);
    pending_height.store(new_size.height, std::memory_order_relaxed);
    resize_pending.store(true, std::memory_order_release);
    Invalidate();
  }
#else
  if (buffer.IsDefined()) {
    buffer.Resize(new_size);
    Invalidate();
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

#if defined(USE_MEMORY_CANVAS) || defined(ENABLE_OPENGL)
  if (resize_pending.exchange(false, std::memory_order_acquire)) {
    const PixelSize new_size{
      pending_width.load(std::memory_order_relaxed),
      pending_height.load(std::memory_order_relaxed),
    };
    buffer.Resize(new_size);
    dirty = true;
  }
#endif

#ifdef ENABLE_OPENGL
  if (auto *top = dynamic_cast<UI::TopWindow *>(GetRootOwner())) {
    const uint32_t current_render_state_token = top->GetRenderStateToken();
    if (!render_state_token_known ||
        current_render_state_token != last_render_state_token) {
      dirty = true;
      last_render_state_token = current_render_state_token;
      render_state_token_known = true;
    }
  }

  /* Keep cached content, but always use Begin()/Commit() to keep OpenGL
     viewport/orientation state synchronized after display rotation changes. */
  buffer.Begin(canvas);
  if (dirty) {
    dirty = false;
    OnPaintBuffer(buffer);
  }
  buffer.Commit(canvas);

#else

  if (dirty) {
    dirty = false;
    OnPaintBuffer(buffer);
  }

  canvas.Copy(buffer);
#endif
}
