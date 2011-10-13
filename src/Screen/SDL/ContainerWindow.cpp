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

#include "Screen/ContainerWindow.hpp"
#include "Screen/SubCanvas.hpp"
#include "Screen/SDL/Reference.hpp"

#include <algorithm>
#include <assert.h>

ContainerWindow::ContainerWindow()
  :active_child(NULL), capture_child(NULL)
{
}

ContainerWindow::~ContainerWindow()
{
  reset();
}

gcc_pure
Window *
ContainerWindow::find_control(std::list<Window*>::const_iterator i,
                              std::list<Window*>::const_iterator end)
{
  for (; i != end; ++i) {
    Window &child = **i;
    if (!child.is_visible() || !child.is_enabled())
      continue;

    if (child.is_tab_stop())
      return &child;

    if (child.is_control_parent()) {
      ContainerWindow &container = (ContainerWindow &)child;
      Window *control = container.find_first_control();
      if (control != NULL)
        return control;
    }
  }

  return NULL;
}

gcc_pure
Window *
ContainerWindow::find_control(std::list<Window*>::const_reverse_iterator i,
                              std::list<Window*>::const_reverse_iterator end)
{
  for (; i != end; ++i) {
    Window &child = **i;
    if (!child.is_visible() || !child.is_enabled())
      continue;

    if (child.is_tab_stop())
      return &child;

    if (child.is_control_parent()) {
      ContainerWindow &container = (ContainerWindow &)child;
      Window *control = container.find_last_control();
      if (control != NULL)
        return control;
    }
  }

  return NULL;
}

Window *
ContainerWindow::find_first_control()
{
  return find_control(children.begin(), children.end());
}

Window *
ContainerWindow::find_last_control()
{
  return find_control(children.rbegin(), children.rend());
}

Window *
ContainerWindow::find_next_child_control(Window *reference)
{
  assert(reference != NULL);
  assert(reference->GetParent() == this);

  std::list<Window*>::const_iterator i =
    std::find(children.begin(), children.end(), reference);
  assert(i != children.end());

  return find_control(++i, children.end());
}

Window *
ContainerWindow::find_previous_child_control(Window *reference)
{
  assert(reference != NULL);
  assert(reference->GetParent() == this);

  std::list<Window*>::const_reverse_iterator i =
    std::find(children.rbegin(), children.rend(), reference);
#ifndef ANDROID
  /* Android's NDK r5b ships a cxx-stl which does not allow comparing
     two const_reverse_iterator objects for inequality */
  assert(i != children.rend());
#endif

  return find_control(++i, children.rend());
}

Window *
ContainerWindow::find_next_control(Window *reference)
{
  assert(reference != NULL);

  if (reference == this)
    return NULL;

  while (true) {
    ContainerWindow *container = reference->parent;
    assert(container != NULL);

    Window *control = container->find_next_child_control(reference);
    if (control != NULL)
      return control;

    if (container == this)
      return NULL;

    reference = container;
  }
}

Window *
ContainerWindow::find_previous_control(Window *reference)
{
  assert(reference != NULL);

  if (reference == this)
    return NULL;

  while (true) {
    ContainerWindow *container = reference->parent;
    assert(container != NULL);

    Window *control = container->find_previous_child_control(reference);
    if (control != NULL)
      return control;

    if (container == this)
      return NULL;

    reference = container;
  }
}

void
ContainerWindow::focus_first_control()
{
  Window *control = find_first_control();
  if (control != NULL)
    control->set_focus();
}

void
ContainerWindow::focus_next_control()
{
  Window *focused = get_focused_window();
  if (focused != NULL) {
    Window *control = find_next_control(focused);
    if (control != NULL)
      control->set_focus();
    else
      focus_first_control();
  } else
    focus_first_control();
}

void
ContainerWindow::focus_previous_control()
{
  Window *focused = get_focused_window();
  Window *control = focused != NULL
    ? find_previous_control(focused)
    : NULL;
  if (control == NULL)
    control = find_last_control();
  if (control != NULL)
    control->set_focus();
}

bool
ContainerWindow::on_destroy()
{
  /* destroy all child windows */
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
ContainerWindow::on_mouse_move(PixelScalar x, PixelScalar y, unsigned keys)
{
  Window *child = event_child_at(x, y);
  if (child != NULL) {
    child->on_mouse_move(x - child->get_left(), y - child->get_top(), keys);
    return true;
  }

  return PaintWindow::on_mouse_move(x, y, keys);
}

bool
ContainerWindow::on_mouse_down(PixelScalar x, PixelScalar y)
{
  Window *child = event_child_at(x, y);
  if (child != NULL) {
    child->on_mouse_down(x - child->get_left(), y - child->get_top());
    return true;
  }

  return PaintWindow::on_mouse_down(x, y);
}

bool
ContainerWindow::on_mouse_up(PixelScalar x, PixelScalar y)
{
  Window *child = event_child_at(x, y);
  if (child != NULL) {
    child->on_mouse_up(x - child->get_left(), y - child->get_top());
    return true;
  }

  return PaintWindow::on_mouse_up(x, y);
}

bool
ContainerWindow::on_mouse_double(PixelScalar x, PixelScalar y)
{
  Window *child = event_child_at(x, y);
  if (child != NULL) {
    child->on_mouse_double(x - child->get_left(), y - child->get_top());
    return true;
  }

  return PaintWindow::on_mouse_double(x, y);
}

bool
ContainerWindow::on_mouse_wheel(PixelScalar x, PixelScalar y, int delta)
{
  Window *child = event_child_at(x, y);
  if (child != NULL) {
    child->on_mouse_wheel(x - child->get_left(), y - child->get_top(), delta);
    return true;
  }

  return PaintWindow::on_mouse_wheel(x, y, delta);
}

void
ContainerWindow::on_paint(Canvas &canvas)
{
  Window *full = NULL;

  const std::list<Window*> &children = this->children;

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

  if (HasBorder()) {
    canvas.black_pen();
    canvas.hollow_brush();
    canvas.rectangle(0, 0, get_width() - 1, get_height() - 1);
  }
}

void
ContainerWindow::add_child(Window &child) {
  children.push_back(&child);

  invalidate();
}

void
ContainerWindow::remove_child(Window &child) {
  children.remove(&child);

  if (active_child == &child)
    active_child = NULL;

  invalidate();
}

bool
ContainerWindow::HasChild(const Window &child) const
{
  return std::find(children.begin(), children.end(), &child) != children.end();
}

void
ContainerWindow::bring_child_to_top(Window &child) {
  children.remove(&child);
  children.insert(children.begin(), &child);
  invalidate();
}

Window *
ContainerWindow::child_at(PixelScalar x, PixelScalar y)
{
  for (std::list<Window*>::const_iterator i = children.begin();
       i != children.end(); ++i) {
    Window &child = **i;
    if (child.is_visible() &&
        x >= child.get_left() && x < child.get_right() &&
        y >= child.get_top() && y < child.get_bottom())
      return child.is_enabled() ? &child : NULL;
  }

  return NULL;
}

Window *
ContainerWindow::event_child_at(PixelScalar x, PixelScalar y)
{
  if (capture)
    /* if this window is capturing the mouse, events must go exactly
       here */
    return NULL;
  else if (capture_child != NULL)
    /* all mouse events go to the capturing child */
    return capture_child;
  else
    /* find the child window at the specified position */
    return child_at(x, y);
}

void
ContainerWindow::set_active_child(Window &child)
{
  if (active_child == &child)
    return;

  ClearFocus();

  active_child = &child;

  if (parent != NULL)
    parent->set_active_child(*this);
}

void
ContainerWindow::set_focus()
{
  /* just in case our child window was focused previously, we must
     clear it now */
  ClearFocus();

  Window::set_focus();
}

void
ContainerWindow::ClearFocus()
{
  if (active_child != NULL) {
    /* clear the focus recursively, until the focused window is
       found */
    active_child->ClearFocus();
    active_child = NULL;
  }

  Window::ClearFocus();
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

WindowReference
ContainerWindow::GetFocusedWindowReference()
{
  Window *focus = get_focused_window();
  return focus != NULL
    ? WindowReference(*this, *focus)
    : WindowReference();
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
