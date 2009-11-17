/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifdef ENABLE_SDL
#include <list>
#endif /* !ENABLE_SDL */

/**
 * A container for more #Window objects.  It is also derived from
 * #PaintWindow, because you might want to paint a border between the
 * child windows.
 */
class ContainerWindow : public PaintWindow {
protected:
#ifdef ENABLE_SDL
  std::list<Window*> children;

  /**
   * The active child window is used to find the focused window.  If
   * this attribute is NULL, then the focused window is not an
   * (indirect) child window of this one.
   */
  Window *active_child;

public:
  ContainerWindow();
#endif /* ENABLE_SDL */

protected:
  virtual Brush *on_color(Window &window, Canvas &canvas);

#ifdef ENABLE_SDL
  virtual bool on_destroy();
  virtual bool on_mouse_move(int x, int y, unsigned keys);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
  virtual bool on_mouse_double(int x, int y);
  virtual void on_paint(Canvas &canvas);
#else /* !ENABLE_SDL */
  virtual LRESULT on_message(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);
#endif

#ifdef ENABLE_SDL
public:
  void add_child(Window &child) {
    children.push_back(&child);
  }

  void remove_child(Window &child) {
    children.remove(&child);

    if (active_child == &child)
      active_child = NULL;
  }

  /**
   * Locate a child window by its relative coordinates.
   */
  Window *child_at(int x, int y);

  void set_active_child(Window &child);

  /**
   * Override the Window::get_focused_window() method, and search in
   * the active child window.
   */
  virtual Window *get_focused_window();

  void expose_child(const Window &child) {
    canvas.copy(child.get_left(), child.get_top(),
                child.get_canvas().get_width(),
                child.get_canvas().get_height(),
                child.get_canvas(), 0, 0);
    expose(child.get_position());
  }
#endif /* ENABLE_SDL */
};

#endif
