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
#include "Screen/SubCanvas.hpp"

#include <assert.h>

ContainerWindow::ContainerWindow()
  :active_child(NULL), capture_child(NULL)
{
}

ContainerWindow::~ContainerWindow()
{
  reset();
}

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
  Window *child = event_child_at(x, y);
  if (child != NULL) {
    child->on_mouse_move(x - child->get_left(), y - child->get_top(), keys);
    return true;
  }

  return PaintWindow::on_mouse_move(x, y, keys);
}

bool
ContainerWindow::on_mouse_down(int x, int y)
{
  Window *child = event_child_at(x, y);
  if (child != NULL) {
    child->on_mouse_down(x - child->get_left(), y - child->get_top());
    return true;
  }

  return PaintWindow::on_mouse_down(x, y);
}

bool
ContainerWindow::on_mouse_up(int x, int y)
{
  Window *child = event_child_at(x, y);
  if (child != NULL) {
    child->on_mouse_up(x - child->get_left(), y - child->get_top());
    return true;
  }

  return PaintWindow::on_mouse_up(x, y);
}

bool
ContainerWindow::on_mouse_double(int x, int y)
{
  Window *child = event_child_at(x, y);
  if (child != NULL) {
    child->on_mouse_double(x - child->get_left(), y - child->get_top());
    return true;
  }

  return PaintWindow::on_mouse_double(x, y);
}

void
ContainerWindow::on_paint(Canvas &canvas)
{
  Window *full = NULL;

  /* find the last full window, which covers all the other windows
     behind it */
  for (std::list<Window*>::const_reverse_iterator i = children.rbegin();
       i != children.rend(); ++i) {
    Window &child = **i;
    if (child.is_visible() &&
        child.get_left() <= 0 && child.get_right() >= (int)get_width() &&
        child.get_top() <= 0 && child.get_bottom() >= (int)get_height())
      full = &child;
  }

  for (std::list<Window*>::const_reverse_iterator i = children.rbegin();
       i != children.rend(); ++i) {
    Window &child = **i;
    if (!child.is_visible())
      continue;

    if (full != NULL) {
      if (&child == full)
        full = NULL;
      else
        /* don't bother to draw the children "behind" the last full
           window */
        continue;
    }

    SubCanvas sub_canvas(canvas, child.get_left(), child.get_top(),
                         child.get_width(), child.get_height());
    child.setup(sub_canvas);
    child.on_paint(sub_canvas);
  }

  assert(full == NULL);
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

Window *
ContainerWindow::event_child_at(int x, int y)
{
  if (capture)
    return NULL;
  else if (capture_child != NULL)
    return capture_child;
  else
    return child_at(x, y);
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

void
ContainerWindow::set_child_capture(Window *window)
{
  assert(window != NULL);
  assert(window->parent == this);

  if (capture_child == window)
    return;

  if (capture_child != NULL)
    clear_capture();

  capture_child = window;
  if (parent != NULL)
    parent->set_child_capture(this);
}

void
ContainerWindow::release_child_capture(Window *window)
{
  assert(window != NULL);
  assert(window->parent == this);

  if (capture_child != window)
    return;

  capture_child = NULL;

  if (parent != NULL)
    parent->release_child_capture(this);
}

void
ContainerWindow::clear_capture()
{
  Window::clear_capture();

  if (capture_child != NULL) {
    capture_child->clear_capture();
    capture_child = NULL;
  }
}
