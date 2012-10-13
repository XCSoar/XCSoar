/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
ContainerWindow::FindControl(std::list<Window*>::const_iterator i,
                             std::list<Window*>::const_iterator end)
{
  for (; i != end; ++i) {
    Window &child = **i;
    if (!child.IsVisible() || !child.IsEnabled())
      continue;

    if (child.IsTabStop())
      return &child;

    if (child.IsControlParent()) {
      ContainerWindow &container = (ContainerWindow &)child;
      Window *control = container.FindFirstControl();
      if (control != NULL)
        return control;
    }
  }

  return NULL;
}

gcc_pure
Window *
ContainerWindow::FindControl(std::list<Window*>::const_reverse_iterator i,
                             std::list<Window*>::const_reverse_iterator end)
{
  for (; i != end; ++i) {
    Window &child = **i;
    if (!child.IsVisible() || !child.IsEnabled())
      continue;

    if (child.IsTabStop())
      return &child;

    if (child.IsControlParent()) {
      ContainerWindow &container = (ContainerWindow &)child;
      Window *control = container.FindLastControl();
      if (control != NULL)
        return control;
    }
  }

  return NULL;
}

Window *
ContainerWindow::FindFirstControl()
{
  return FindControl(children.begin(), children.end());
}

Window *
ContainerWindow::FindLastControl()
{
  return FindControl(children.rbegin(), children.rend());
}

Window *
ContainerWindow::FindNextChildControl(Window *reference)
{
  assert(reference != NULL);
  assert(reference->GetParent() == this);

  std::list<Window*>::const_iterator i =
    std::find(children.begin(), children.end(), reference);
  assert(i != children.end());

  return FindControl(++i, children.end());
}

Window *
ContainerWindow::FindPreviousChildControl(Window *reference)
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

  return FindControl(++i, children.rend());
}

Window *
ContainerWindow::FindNextControl(Window *reference)
{
  assert(reference != NULL);

  if (reference == this)
    return NULL;

  while (true) {
    ContainerWindow *container = reference->parent;
    assert(container != NULL);

    Window *control = container->FindNextChildControl(reference);
    if (control != NULL)
      return control;

    if (container == this)
      return NULL;

    reference = container;
  }
}

Window *
ContainerWindow::FindPreviousControl(Window *reference)
{
  assert(reference != NULL);

  if (reference == this)
    return NULL;

  while (true) {
    ContainerWindow *container = reference->parent;
    assert(container != NULL);

    Window *control = container->FindPreviousChildControl(reference);
    if (control != NULL)
      return control;

    if (container == this)
      return NULL;

    reference = container;
  }
}

bool
ContainerWindow::FocusFirstControl()
{
  Window *control = FindFirstControl();
  if (control == NULL)
    return false;

  control->SetFocus();
  return true;
}

bool
ContainerWindow::FocusNextControl()
{
  Window *focused = GetFocusedWindow();
  if (focused != NULL) {
    Window *control = FindNextControl(focused);
    if (control != NULL) {
      control->SetFocus();
      return true;
    } else
      return FocusFirstControl();
  } else
    return FocusFirstControl();
}

bool
ContainerWindow::FocusPreviousControl()
{
  Window *focused = GetFocusedWindow();
  Window *control = focused != NULL
    ? FindPreviousControl(focused)
    : NULL;
  if (control == NULL) {
    control = FindLastControl();
    if (control == NULL)
      return false;
  }

  control->SetFocus();
  return true;
}

void
ContainerWindow::OnDestroy()
{
  /* destroy all child windows */
  std::list<Window*>::const_iterator i;
  while ((i = children.begin()) != children.end()) {
    Window *w = *i;
    w->reset();

    assert(std::find(children.begin(), children.end(), w) == children.end());
  }

  assert(children.empty());

  PaintWindow::OnDestroy();
}

bool
ContainerWindow::OnCancelMode()
{
  bool result = Window::OnCancelMode();

  if (active_child != NULL)
    result |= active_child->OnCancelMode();

  if (capture_child != NULL)
    result |= capture_child->OnCancelMode();

  return result;
}

bool
ContainerWindow::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  Window *child = EventChildAt(x, y);
  if (child != NULL) {
    child->OnMouseMove(x - child->GetLeft(), y - child->GetTop(), keys);
    return true;
  }

  return PaintWindow::OnMouseMove(x, y, keys);
}

bool
ContainerWindow::OnMouseDown(PixelScalar x, PixelScalar y)
{
  Window *child = EventChildAt(x, y);
  if (child != NULL) {
    child->OnMouseDown(x - child->GetLeft(), y - child->GetTop());
    return true;
  }

  return PaintWindow::OnMouseDown(x, y);
}

bool
ContainerWindow::OnMouseUp(PixelScalar x, PixelScalar y)
{
  Window *child = EventChildAt(x, y);
  if (child != NULL) {
    child->OnMouseUp(x - child->GetLeft(), y - child->GetTop());
    return true;
  }

  return PaintWindow::OnMouseUp(x, y);
}

bool
ContainerWindow::OnMouseDouble(PixelScalar x, PixelScalar y)
{
  Window *child = EventChildAt(x, y);
  if (child != NULL) {
    child->OnMouseDouble(x - child->GetLeft(), y - child->GetTop());
    return true;
  }

  return PaintWindow::OnMouseDouble(x, y);
}

bool
ContainerWindow::OnMouseWheel(PixelScalar x, PixelScalar y, int delta)
{
  Window *child = EventChildAt(x, y);
  if (child != NULL) {
    child->OnMouseWheel(x - child->GetLeft(), y - child->GetTop(), delta);
    return true;
  }

  return PaintWindow::OnMouseWheel(x, y, delta);
}

#ifdef HAVE_MULTI_TOUCH

bool
ContainerWindow::OnMultiTouchDown()
{
  if (!capture && capture_child != NULL) {
    capture_child->OnMultiTouchDown();
    return true;
  }

  return PaintWindow::OnMultiTouchDown();
}

bool
ContainerWindow::OnMultiTouchUp()
{
  if (!capture && capture_child != NULL) {
    capture_child->OnMultiTouchUp();
    return true;
  }

  return PaintWindow::OnMultiTouchUp();
}

#endif /* HAVE_MULTI_TOUCH */

void
ContainerWindow::OnPaint(Canvas &canvas)
{
  Window *full = NULL;

  const std::list<Window*> &children = this->children;

  /* find the last full window, which covers all the other windows
     behind it */
  for (auto i = children.rbegin(); i != children.rend(); ++i) {
    Window &child = **i;
    if (child.IsVisible() &&
        child.GetLeft() <= 0 && child.GetRight() >= (int)GetWidth() &&
        child.GetTop() <= 0 && child.GetBottom() >= (int)GetHeight())
      full = &child;
  }

  for (auto i = children.rbegin(); i != children.rend(); ++i) {
    Window &child = **i;
    if (!child.IsVisible())
      continue;

    if (full != NULL) {
      if (&child == full)
        full = NULL;
      else
        /* don't bother to draw the children "behind" the last full
           window */
        continue;
    }

    SubCanvas sub_canvas(canvas, child.GetLeft(), child.GetTop(),
                         child.GetWidth(), child.GetHeight());
    child.Setup(sub_canvas);
    child.OnPaint(sub_canvas);
  }

  assert(full == NULL);

  if (HasBorder())
    canvas.DrawOutlineRectangle(-1, -1, GetWidth(), GetHeight(),
                                COLOR_BLACK);
}

void
ContainerWindow::AddChild(Window &child) {
  children.push_back(&child);

  Invalidate();
}

void
ContainerWindow::Removehild(Window &child) {
  children.remove(&child);

  if (active_child == &child)
    active_child = NULL;

  Invalidate();
}

bool
ContainerWindow::HasChild(const Window &child) const
{
  return std::find(children.begin(), children.end(), &child) != children.end();
}

void
ContainerWindow::BringChildToTop(Window &child) {
  children.remove(&child);
  children.insert(children.begin(), &child);
  Invalidate();
}

Window *
ContainerWindow::ChildAt(PixelScalar x, PixelScalar y)
{
  for (auto i = children.begin(); i != children.end(); ++i) {
    Window &child = **i;
    if (child.IsVisible() &&
        x >= child.GetLeft() && x < child.GetRight() &&
        y >= child.GetTop() && y < child.GetBottom())
      return child.IsEnabled() ? &child : NULL;
  }

  return NULL;
}

Window *
ContainerWindow::EventChildAt(PixelScalar x, PixelScalar y)
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
    return ChildAt(x, y);
}

void
ContainerWindow::SetActiveChild(Window &child)
{
  if (active_child == &child)
    return;

  ClearFocus();

  active_child = &child;

  if (parent != NULL)
    parent->SetActiveChild(*this);
}

void
ContainerWindow::SetFocus()
{
  /* just in case our child window was focused previously, we must
     clear it now */
  ClearFocus();

  Window::SetFocus();
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
ContainerWindow::GetFocusedWindow()
{
  Window *window = PaintWindow::GetFocusedWindow();
  if (window != NULL)
    return window;

  if (active_child != NULL)
    return active_child->GetFocusedWindow();

  return NULL;
}

WindowReference
ContainerWindow::GetFocusedWindowReference()
{
  Window *focus = GetFocusedWindow();
  return focus != NULL
    ? WindowReference(*this, *focus)
    : WindowReference();
}

void
ContainerWindow::SetChildCapture(Window *window)
{
  assert(window != NULL);
  assert(window->parent == this);

  if (capture_child == window)
    return;

  if (capture_child != NULL)
    ClearCapture();

  capture_child = window;
  if (parent != NULL)
    parent->SetChildCapture(this);
}

void
ContainerWindow::ReleaseChildCapture(Window *window)
{
  assert(window != NULL);
  assert(window->parent == this);

  if (capture_child != window)
    return;

  capture_child = NULL;

  if (parent != NULL)
    parent->ReleaseChildCapture(this);
}

void
ContainerWindow::ClearCapture()
{
  Window::ClearCapture();

  if (capture_child != NULL) {
    capture_child->ClearCapture();
    capture_child = NULL;
  }
}
