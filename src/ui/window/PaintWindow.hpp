// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Window.hpp"

#ifdef USE_WINUSER
#endif

class ContainerWindow;

/**
 * A #Window implementation for custom drawing.  Implement
 * Window::OnPaint() to draw something.
 */
class PaintWindow : public Window {
public:
#ifdef USE_WINUSER
  static bool register_class(HINSTANCE hInstance) noexcept;
#endif

#ifndef USE_WINUSER
  using Window::Create;

  void Create(ContainerWindow &parent, PixelRect rc,
              const WindowStyle style=WindowStyle()) noexcept {
    Create(&parent, rc, style);
  }
#else /* USE_WINUSER */
  void Create(ContainerWindow *parent, const char *cls, PixelRect rc,
              const WindowStyle style=WindowStyle()) noexcept {
    Window::Create(parent, cls, nullptr, rc, style);
  }

  void Create(ContainerWindow &parent, const char *cls, PixelRect rc,
              const WindowStyle style=WindowStyle()) noexcept {
    Create(&parent, cls, rc, style);
  }

  void Create(ContainerWindow &parent, PixelRect rc,
              const WindowStyle style=WindowStyle()) noexcept {
    Create(parent, "PaintWindow", rc, style);
  }
#endif /* USE_WINUSER */

  constexpr
  static bool SupportsPartialRedraw() noexcept {
#ifdef USE_WINUSER
    /* we can use the GDI function InvalidateRect() with a non-nullptr
       RECT */
    return true;
#else
    /* SDL and OpenGL can't do partial redraws, they always repaint
       the whole screen */
    return false;
#endif
  }

  /**
   * Invalidates the visible area and schedules a repaint (which will
   * occur in the main thread).
   */
  void Invalidate() noexcept {
    AssertThread();

#ifndef USE_WINUSER
    Window::Invalidate();
#else
    ::InvalidateRect(hWnd, nullptr, false);
#endif
  }

  /**
   * Invalidates a part of the visible area and schedules a repaint
   * (which will occur in the main thread).
   */
  void Invalidate([[maybe_unused]] const PixelRect &rect) noexcept {
#ifndef USE_WINUSER
    Invalidate();
#else
    const RECT r = rect;
    ::InvalidateRect(hWnd, &r, false);
#endif
  }

#ifdef USE_WINUSER
protected:
  /* virtual methods from class Window */
  LRESULT OnMessage(HWND hWnd, UINT message,
                    WPARAM wParam, LPARAM lParam) noexcept override;
#endif

  /* virtual methods from class PaintWindow */
  virtual void OnPaint(Canvas &canvas) noexcept = 0;

  virtual void OnPaint(Canvas &canvas,
                       [[maybe_unused]] const PixelRect &dirty) noexcept {
    OnPaint(canvas);
  }
};
