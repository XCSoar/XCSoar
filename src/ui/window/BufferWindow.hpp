// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PaintWindow.hpp"
#include "ui/canvas/BufferCanvas.hpp"

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
