// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PaintWindow.hpp"

#ifndef USE_WINUSER
#include "custom/WList.hpp"
#endif

#ifdef USE_WINUSER
class Brush;
#else
class WindowReference;
#endif

/**
 * A container for more #Window objects.  It is also derived from
 * #PaintWindow, because you might want to paint a border between the
 * child windows.
 */
class ContainerWindow : public PaintWindow {
protected:
#ifndef USE_WINUSER
  friend class WindowList;
  WindowList children;

  /**
   * The active child window is used to find the focused window.  If
   * this attribute is nullptr, then the focused window is not an
   * (indirect) child window of this one.
   */
  Window *active_child = nullptr;

  /**
   * The child window which captures the mouse.
   */
  Window *capture_child = nullptr;

public:
  ~ContainerWindow() noexcept override;
#endif /* !USE_WINUSER */

protected:
#ifndef USE_WINUSER
  void OnDestroy() noexcept override;
  void OnCancelMode() noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseDouble(PixelPoint p) noexcept override;
  bool OnMouseWheel(PixelPoint p, int delta) noexcept override;

#ifdef HAVE_MULTI_TOUCH
  bool OnMultiTouchDown() noexcept override;
  bool OnMultiTouchUp() noexcept override;
#endif

  void OnPaint(Canvas &canvas) noexcept override;
#else /* USE_WINUSER */
  virtual void OnPaint([[maybe_unused]] Canvas &canvas) noexcept {}
#endif

public:
#ifndef USE_WINUSER
  void AddChild(Window &child) noexcept;
  void RemoveChild(Window &child) noexcept;

  [[gnu::pure]]
  bool HasChild(const Window &w) const noexcept {
    return children.Contains(w);
  }

  /**
   * Like Invalidate(), but if the specified window is covered by a
   * sibling, this method is a no-op.
   */
  void InvalidateChild(const Window &child) noexcept;

  void BringChildToTop(Window &child) noexcept {
    children.BringToTop(child);
    InvalidateChild(child);
  }

  void BringChildToBottom(Window &child) noexcept {
    children.BringToBottom(child);
    Invalidate();
  }

  /**
   * Locate a child window by its relative coordinates.
   */
  [[gnu::pure]]
  Window *ChildAt(PixelPoint p) noexcept {
    return children.FindAt(p);
  }

  /**
   * Locates the child which should get a mouse event.  Prefers the
   * captured child.
   */
  [[gnu::pure]]
  Window *EventChildAt(PixelPoint p) noexcept;

  void SetActiveChild(Window &child) noexcept;
  void SetFocus() noexcept override;
  void ClearFocus() noexcept override;

  /**
   * Override the Window::GetFocusedWindow() method, and search in
   * the active child window.
   */
  [[gnu::pure]]
  Window *GetFocusedWindow() noexcept override;

  [[gnu::pure]]
  WindowReference GetFocusedWindowReference() noexcept;

  void SetChildCapture(Window *window) noexcept;
  void ReleaseChildCapture(Window *window) noexcept;
  void ClearCapture() noexcept override;

protected:
  [[gnu::pure]]
  Window *FindNextControl(Window *reference) noexcept;

  [[gnu::pure]]
  Window *FindPreviousControl(Window *reference) noexcept;

public:
#endif /* !USE_WINUSER */

  /**
   * Sets the keyboard focus on the first descendant window which has
   * the WindowStyle::tab_stop() attribute.
   *
   * @return true if the focus has been moved
   */
  bool FocusFirstControl() noexcept;

  /**
   * Sets the keyboard focus on the next descendant window which has
   * the WindowStyle::tab_stop() attribute.
   *
   * @return true if the focus has been moved
   */
  bool FocusNextControl() noexcept;

  /**
   * Sets the keyboard focus on the previous descendant window which
   * has the WindowStyle::tab_stop() attribute.
   *
   * @return true if the focus has been moved
   */
  bool FocusPreviousControl() noexcept;

  /**
   * If this is a scrollable window, then attempt to make the given
   * rectangle visible in the view port.
   */
  virtual void ScrollTo(const PixelRect &rc) noexcept;
};
