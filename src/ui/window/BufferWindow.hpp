// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PaintWindow.hpp"
#include "ui/canvas/BufferCanvas.hpp"

#include <atomic>

#ifdef ENABLE_OPENGL
#include <cstdint>
#endif

/**
 * A #PaintWindow with buffered painting, to avoid flickering.
 */
class BufferWindow : public PaintWindow {
  BufferCanvas buffer;

  /**
   * Is the buffer dirty, i.e. does it need a full repaint with
   * OnPaintBuffer()?
   */
  bool dirty;

#ifdef ENABLE_OPENGL
  uint32_t last_render_state_token = 0;
  bool render_state_token_known = false;
#endif

#if defined(USE_MEMORY_CANVAS) || defined(ENABLE_OPENGL)
  /**
   * Pending resize applied at the start of the next #OnPaint() (never
   * while the buffer FBO is active).
   */
  std::atomic<bool> resize_pending{false};
  std::atomic<unsigned> pending_width{0};
  std::atomic<unsigned> pending_height{0};
#endif

public:
  void Invalidate() noexcept
#ifndef USE_WINUSER
    override
#endif
  {
    dirty = true;
    PaintWindow::Invalidate();
  }

  void Invalidate(const PixelRect &rect) noexcept {
    dirty = true;
    PaintWindow::Invalidate(rect);
  }

protected:
  /**
   * Determines whether this BufferWindow maintains a persistent
   * buffer which allows incremental drawing in each frame.
   */
  static constexpr bool IsPersistent() noexcept {
    return true;
  }

protected:
  /* virtual methods from class Window */
  void OnResize(PixelSize new_size) noexcept override;

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;

  /* our virtual methods */
  virtual void OnPaintBuffer(Canvas &canvas) noexcept = 0;
};
