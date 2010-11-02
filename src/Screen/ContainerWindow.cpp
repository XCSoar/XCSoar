/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Screen/ContainerWindow.hpp"

#ifndef ENABLE_SDL
#include "Screen/ButtonWindow.hpp"
#endif

#include <assert.h>

#ifdef ENABLE_SDL

#include "Screen/SubCanvas.hpp"

ContainerWindow::ContainerWindow()
  :active_child(NULL)
{
}

ContainerWindow::~ContainerWindow()
{
  reset();
}

#endif /* ENABLE_SDL */

Brush *
ContainerWindow::on_color(Window &window, Canvas &canvas)
{
  return NULL;
}

#ifdef ENABLE_SDL

bool
ContainerWindow::on_destroy()
{
  std::list<Window*>::const_iterator i;
  while ((i = children.begin()) != children.end()) {
    Window *w = *i;
    w->reset();

    assert(std::find(children.begin(), children.end(), w) == children.end());
  }

  assert(children.empty());

  PaintWindow::on_destroy();
  return true;
}

bool
ContainerWindow::on_mouse_move(int x, int y, unsigned keys)
{
  Window *child = child_at(x, y);
  if (child != NULL) {
    child->on_mouse_move(x - child->get_left(), y - child->get_top(), keys);
    return true;
  }

  return PaintWindow::on_mouse_move(x, y, keys);
}

bool
ContainerWindow::on_mouse_down(int x, int y)
{
  Window *child = child_at(x, y);
  if (child != NULL) {
    child->on_mouse_down(x - child->get_left(), y - child->get_top());
    return true;
  }

  return PaintWindow::on_mouse_down(x, y);
}

bool
ContainerWindow::on_mouse_up(int x, int y)
{
  Window *child = child_at(x, y);
  if (child != NULL) {
    child->on_mouse_up(x - child->get_left(), y - child->get_top());
    return true;
  }

  return PaintWindow::on_mouse_up(x, y);
}

bool
ContainerWindow::on_mouse_double(int x, int y)
{
  Window *child = child_at(x, y);
  if (child != NULL) {
    child->on_mouse_double(x - child->get_left(), y - child->get_top());
    return true;
  }

  return PaintWindow::on_mouse_double(x, y);
}

void
ContainerWindow::on_paint(Canvas &canvas)
{
  for (std::list<Window*>::const_reverse_iterator i = children.rbegin();
       i != children.rend(); ++i) {
    Window &child = **i;
    if (!child.is_visible())
      continue;

    SubCanvas sub_canvas(canvas, child.get_left(), child.get_top(),
                         child.get_width(), child.get_height());
    child.setup(sub_canvas);
    child.on_paint(sub_canvas);
  }
}


Window *
ContainerWindow::child_at(int x, int y)
{
  for (std::list<Window*>::const_iterator i = children.begin();
       i != children.end(); ++i) {
    Window &child = **i;
    if (child.is_visible() &&
        x >= child.get_left() && x < child.get_right() &&
        y >= child.get_top() && y < child.get_bottom())
      return &child;
  }

  return NULL;
}

void
ContainerWindow::set_active_child(Window &child)
{
  if (active_child == &child)
    return;

  Window *focus = get_focused_window();
  if (focus != NULL)
    focus->on_killfocus();

  active_child = &child;

  if (parent != NULL)
    parent->set_active_child(*this);
}

Window *
ContainerWindow::get_focused_window()
{
  Window *window = PaintWindow::get_focused_window();
  if (window != NULL)
    return window;

  if (active_child != NULL)
    return active_child->get_focused_window();

  return NULL;
}

#else /* !ENABLE_SDL */

LRESULT
ContainerWindow::on_message(HWND hWnd, UINT message,
                            WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_CTLCOLORSTATIC:
  case WM_CTLCOLORBTN:
    {
      Window *window = Window::get((HWND)lParam);
      if (window == NULL)
        break;

      Canvas canvas((HDC)wParam, 1, 1);
      Brush *brush = on_color(*window, canvas);
      if (brush == NULL)
        break;

      return (LRESULT)brush->native();
    }

  case WM_DRAWITEM:
    /* forward WM_DRAWITEM to the child window who sent this
       message */
    {
      const DRAWITEMSTRUCT *di = (const DRAWITEMSTRUCT *)lParam;

      Window *window = Window::get(di->hwndItem);
      if (window == NULL)
        break;

      Canvas canvas(di->hDC, di->rcItem.right - di->rcItem.left,
                    di->rcItem.bottom - di->rcItem.top);
      window->on_paint(canvas);
      return TRUE;
    }

  case WM_COMMAND:
    if (wParam == MAKEWPARAM(BaseButtonWindow::COMMAND_BOUNCE_ID, BN_CLICKED)) {
      /* forward this message to ButtonWindow::on_clicked() */
      BaseButtonWindow *window = (BaseButtonWindow *)Window::get((HWND)lParam);
      if (window != NULL && window->on_clicked())
        return true;
    }
    break;
  };

  return PaintWindow::on_message(hWnd, message, wParam, lParam);
}

#endif /* !ENABLE_SDL */

void
ContainerWindow::focus_first_control()
{
#ifdef ENABLE_SDL
  // XXX
#else
  HWND hControl = ::GetNextDlgTabItem(hWnd, hWnd, false);
  if (hControl != NULL)
    ::SetFocus(hControl);
#endif
}

void
ContainerWindow::focus_next_control()
{
#ifdef ENABLE_SDL
  // XXX
#else
  HWND hControl = ::GetNextDlgTabItem(hWnd, ::GetFocus(), false);
  if (hControl != NULL)
    ::SetFocus(hControl);
#endif
}

void
ContainerWindow::focus_previous_control()
{
#ifdef ENABLE_SDL
  // XXX
#else
  HWND hControl = ::GetNextDlgTabItem(hWnd, ::GetFocus(), true);
  if (hControl != NULL)
    ::SetFocus(hControl);
#endif
}
