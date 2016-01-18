/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_CONTAINER_WINDOW_HXX
#define XCSOAR_SCREEN_CONTAINER_WINDOW_HXX

#include "Screen/PaintWindow.hpp"

#ifndef USE_WINUSER
#include "Screen/Custom/WList.hpp"
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
  virtual ~ContainerWindow();
#endif /* !USE_WINUSER */

protected:
#ifndef USE_WINUSER
  void OnDestroy() override;
  void OnCancelMode() override;
  bool OnMouseMove(PixelPoint p, unsigned keys) override;
  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;
  bool OnMouseDouble(PixelPoint p) override;
  bool OnMouseWheel(PixelPoint p, int delta) override;

#ifdef HAVE_MULTI_TOUCH
  bool OnMultiTouchDown() override;
  bool OnMultiTouchUp() override;
#endif

  void OnPaint(Canvas &canvas) override;
#else /* USE_WINUSER */
  virtual void OnPaint(gcc_unused Canvas &canvas) {}
#endif

public:
#ifndef USE_WINUSER
  void AddChild(Window &child);
  void RemoveChild(Window &child);

  gcc_pure
  bool HasChild(const Window &w) const {
    return children.Contains(w);
  }

  /**
   * Like Invalidate(), but if the specified window is covered by a
   * sibling, this method is a no-op.
   */
  void InvalidateChild(const Window &child);

  void BringChildToTop(Window &child) {
    children.BringToTop(child);
    InvalidateChild(child);
  }

  void BringChildToBottom(Window &child) {
    children.BringToBottom(child);
    Invalidate();
  }

  /**
   * Locate a child window by its relative coordinates.
   */
  gcc_pure
  Window *ChildAt(PixelPoint p) {
    return children.FindAt(p);
  }

  /**
   * Locates the child which should get a mouse event.  Prefers the
   * captured child.
   */
  gcc_pure
  Window *EventChildAt(PixelPoint p);

  void SetActiveChild(Window &child);
  void SetFocus() override;
  void ClearFocus() override;

  /**
   * Override the Window::GetFocusedWindow() method, and search in
   * the active child window.
   */
  gcc_pure
  Window *GetFocusedWindow() override;

  gcc_pure
  WindowReference GetFocusedWindowReference();

  void SetChildCapture(Window *window);
  void ReleaseChildCapture(Window *window);
  void ClearCapture() override;

protected:
  gcc_pure
  Window *FindNextControl(Window *reference);

  gcc_pure
  Window *FindPreviousControl(Window *reference);

public:
#endif /* !USE_WINUSER */

  /**
   * Sets the keyboard focus on the first descendant window which has
   * the WindowStyle::tab_stop() attribute.
   *
   * @return true if the focus has been moved
   */
  bool FocusFirstControl();

  /**
   * Sets the keyboard focus on the next descendant window which has
   * the WindowStyle::tab_stop() attribute.
   *
   * @return true if the focus has been moved
   */
  bool FocusNextControl();

  /**
   * Sets the keyboard focus on the previous descendant window which
   * has the WindowStyle::tab_stop() attribute.
   *
   * @return true if the focus has been moved
   */
  bool FocusPreviousControl();
};

#endif
