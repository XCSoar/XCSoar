// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Window.hpp"

class ContainerWindow;

/**
 * A #Window implementation for custom drawing.  Implement
 * Window::OnPaint() to draw something.
 */
class PaintWindow : public Window {
public:
  using Window::Create;

  void Create(ContainerWindow &parent, PixelRect rc,
              const WindowStyle style=WindowStyle()) noexcept {
    Create(&parent, rc, style);
  }

  constexpr
  static bool SupportsPartialRedraw() noexcept {
    /* SDL and OpenGL can't do partial redraws, they always repaint
       the whole screen */
    return false;
  }

  /**
   * Invalidates the visible area and schedules a repaint (which will
   * occur in the main thread).
   */
  void Invalidate() noexcept {
    AssertThread();

    Window::Invalidate();
  }

  /**
   * Invalidates a part of the visible area and schedules a repaint
   * (which will occur in the main thread).
   */
  void Invalidate([[maybe_unused]] const PixelRect &rect) noexcept {
    Invalidate();
  }

public:
  virtual void OnPaint(Canvas &canvas) noexcept = 0;

  virtual void OnPaint(Canvas &canvas,
                       [[maybe_unused]] const PixelRect &dirty) noexcept {
    OnPaint(canvas);
  }
};
