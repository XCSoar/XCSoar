/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef USE_GDI
#include <list>
#endif

class WindowReference;

/**
 * A container for more #Window objects.  It is also derived from
 * #PaintWindow, because you might want to paint a border between the
 * child windows.
 */
class ContainerWindow : public PaintWindow {
protected:
#ifndef USE_GDI
  std::list<Window*> children;

  /**
   * The active child window is used to find the focused window.  If
   * this attribute is NULL, then the focused window is not an
   * (indirect) child window of this one.
   */
  Window *active_child;

  /**
   * The child window which captures the mouse.
   */
  Window *capture_child;

public:
  ContainerWindow();
  virtual ~ContainerWindow();
#endif /* !USE_GDI */

protected:
  virtual const Brush *on_color(Window &window, Canvas &canvas);

#ifndef USE_GDI
  virtual bool on_destroy();
  virtual bool on_mouse_move(PixelScalar x, PixelScalar y, unsigned keys);
  virtual bool on_mouse_down(PixelScalar x, PixelScalar y);
  virtual bool on_mouse_up(PixelScalar x, PixelScalar y);
  virtual bool on_mouse_double(PixelScalar x, PixelScalar y);
  virtual bool on_mouse_wheel(PixelScalar x, PixelScalar y, int delta);
  virtual void on_paint(Canvas &canvas);
#else /* USE_GDI */
  virtual LRESULT on_message(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);
#endif

public:
#ifndef USE_GDI
  void add_child(Window &child);
  void remove_child(Window &child);

  gcc_pure
  bool HasChild(const Window &child) const;

  void bring_child_to_top(Window &child);

  void BringChildToBottom(Window &child) {
    children.remove(&child);
    children.push_back(&child);
    invalidate();
  }

  /**
   * Locate a child window by its relative coordinates.
   */
  gcc_pure
  Window *child_at(PixelScalar x, PixelScalar y);

  /**
   * Locates the child which should get a mouse event.  Prefers the
   * captured child.
   */
  gcc_pure
  Window *event_child_at(PixelScalar x, PixelScalar y);

  void set_active_child(Window &child);
  virtual void set_focus();
  virtual void ClearFocus();

  /**
   * Override the Window::get_focused_window() method, and search in
   * the active child window.
   */
  gcc_pure
  virtual Window *get_focused_window();

  gcc_pure
  WindowReference GetFocusedWindowReference();

  void set_child_capture(Window *window);
  void release_child_capture(Window *window);
  virtual void clear_capture();

protected:
  gcc_pure
  static Window *find_control(std::list<Window*>::const_iterator i,
                              std::list<Window*>::const_iterator end);

  gcc_pure
  static Window *find_control(std::list<Window*>::const_reverse_iterator i,
                              std::list<Window*>::const_reverse_iterator end);

  gcc_pure
  Window *find_first_control();

  gcc_pure
  Window *find_last_control();

  gcc_pure
  Window *find_next_child_control(Window *reference);

  gcc_pure
  Window *find_previous_child_control(Window *reference);

  gcc_pure
  Window *find_next_control(Window *reference);

  gcc_pure
  Window *find_previous_control(Window *reference);

public:
#endif /* !USE_GDI */

  /**
   * Sets the keyboard focus on the first descendant window which has
   * the WindowStyle::tab_stop() attribute.
   */
  void focus_first_control();

  /**
   * Sets the keyboard focus on the next descendant window which has
   * the WindowStyle::tab_stop() attribute.
   */
  void focus_next_control();

  /**
   * Sets the keyboard focus on the previous descendant window which
   * has the WindowStyle::tab_stop() attribute.
   */
  void focus_previous_control();
};

#endif
