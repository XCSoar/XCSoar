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

#include "Screen/ContainerWindow.hpp"
#include "Screen/Canvas.hpp"
#include "Reference.hpp"

#include <algorithm>
#include <assert.h>

ContainerWindow::~ContainerWindow()
{
  Destroy();
}

Window *
ContainerWindow::FindNextControl(Window *reference)
{
  assert(reference != nullptr);

  if (reference == this)
    return nullptr;

  while (true) {
    ContainerWindow *container = reference->parent;
    assert(container != nullptr);

    Window *control = container->children.FindNextChildControl(reference);
    if (control != nullptr)
      return control;

    if (container == this)
      return nullptr;

    reference = container;
  }
}

Window *
ContainerWindow::FindPreviousControl(Window *reference)
{
  assert(reference != nullptr);

  if (reference == this)
    return nullptr;

  while (true) {
    ContainerWindow *container = reference->parent;
    assert(container != nullptr);

    Window *control = container->children.FindPreviousChildControl(reference);
    if (control != nullptr)
      return control;

    if (container == this)
      return nullptr;

    reference = container;
  }
}

bool
ContainerWindow::FocusFirstControl()
{
  Window *control = children.FindFirstControl();
  if (control == nullptr)
    return false;

  control->SetFocus();
  return true;
}

bool
ContainerWindow::FocusNextControl()
{
  Window *focused = GetFocusedWindow();
  if (focused != nullptr) {
    Window *control = FindNextControl(focused);
    if (control != nullptr) {
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
  Window *control = focused != nullptr
    ? FindPreviousControl(focused)
    : nullptr;
  if (control == nullptr) {
    control = children.FindLastControl();
    if (control == nullptr)
      return false;
  }

  control->SetFocus();
  return true;
}

void
ContainerWindow::OnDestroy()
{
  children.Clear();

  PaintWindow::OnDestroy();
}

void
ContainerWindow::OnCancelMode()
{
  Window::OnCancelMode();

  if (active_child != nullptr)
    active_child->OnCancelMode();

  if (capture_child != nullptr)
    capture_child->OnCancelMode();
}

bool
ContainerWindow::OnMouseMove(PixelPoint p, unsigned keys)
{
  Window *child = EventChildAt(p);
  if (child != nullptr)
    return child->OnMouseMove(p - child->GetTopLeft(), keys);

  return PaintWindow::OnMouseMove(p, keys);
}

bool
ContainerWindow::OnMouseDown(PixelPoint p)
{
  Window *child = EventChildAt(p);
  if (child != nullptr)
    return child->OnMouseDown(p - child->GetTopLeft());

  return PaintWindow::OnMouseDown(p);
}

bool
ContainerWindow::OnMouseUp(PixelPoint p)
{
  Window *child = EventChildAt(p);
  if (child != nullptr)
    return child->OnMouseUp(p - child->GetTopLeft());

  return PaintWindow::OnMouseUp(p);
}

bool
ContainerWindow::OnMouseDouble(PixelPoint p)
{
  Window *child = EventChildAt(p);
  if (child != nullptr)
    return child->OnMouseDouble(p - child->GetTopLeft());

  return PaintWindow::OnMouseDouble(p);
}

bool
ContainerWindow::OnMouseWheel(PixelPoint p, int delta)
{
  Window *child = EventChildAt(p);
  if (child != nullptr)
    return child->OnMouseWheel(p - child->GetTopLeft(), delta);

  return PaintWindow::OnMouseWheel(p, delta);
}

#ifdef HAVE_MULTI_TOUCH

bool
ContainerWindow::OnMultiTouchDown()
{
  if (!capture && capture_child != nullptr)
    return capture_child->OnMultiTouchDown();

  return PaintWindow::OnMultiTouchDown();
}

bool
ContainerWindow::OnMultiTouchUp()
{
  if (!capture && capture_child != nullptr)
    return capture_child->OnMultiTouchUp();

  return PaintWindow::OnMultiTouchUp();
}

#endif /* HAVE_MULTI_TOUCH */

void
ContainerWindow::OnPaint(Canvas &canvas)
{
  children.Paint(canvas);

  if (HasBorder())
    canvas.DrawOutlineRectangle(-1, -1, GetWidth(), GetHeight(),
                                COLOR_BLACK);
}

void
ContainerWindow::AddChild(Window &child) {
  children.Add(child);

  InvalidateChild(child);
}

void
ContainerWindow::RemoveChild(Window &child) {
  InvalidateChild(child);

  children.Remove(child);

  if (active_child == &child)
    active_child = nullptr;
}

Window *
ContainerWindow::EventChildAt(PixelPoint p)
{
  if (capture)
    /* if this window is capturing the mouse, events must go exactly
       here */
    return nullptr;
  else if (capture_child != nullptr)
    /* all mouse events go to the capturing child */
    return capture_child;
  else
    /* find the child window at the specified position */
    return ChildAt(p);
}

void
ContainerWindow::InvalidateChild(const Window &child)
{
  AssertThread();

  if (!children.IsCovered(child))
    Invalidate();
}

void
ContainerWindow::SetActiveChild(Window &child)
{
  if (active_child == &child)
    return;

  ClearFocus();

  active_child = &child;

  if (parent != nullptr)
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
  if (active_child != nullptr) {
    /* clear the focus recursively, until the focused window is
       found */
    active_child->ClearFocus();
    active_child = nullptr;
  }

  Window::ClearFocus();
}

Window *
ContainerWindow::GetFocusedWindow()
{
  Window *window = PaintWindow::GetFocusedWindow();
  if (window != nullptr)
    return window;

  if (active_child != nullptr)
    return active_child->GetFocusedWindow();

  return nullptr;
}

WindowReference
ContainerWindow::GetFocusedWindowReference()
{
  Window *focus = GetFocusedWindow();
  return focus != nullptr
    ? WindowReference(*this, *focus)
    : WindowReference();
}

void
ContainerWindow::SetChildCapture(Window *window)
{
  assert(window != nullptr);
  assert(window->parent == this);

  if (capture_child == window)
    return;

  if (capture_child != nullptr)
    ClearCapture();

  capture_child = window;
  if (parent != nullptr)
    parent->SetChildCapture(this);
  else
    EnableCapture();
}

void
ContainerWindow::ReleaseChildCapture(Window *window)
{
  assert(window != nullptr);
  assert(window->parent == this);

  if (capture_child != window)
    return;

  capture_child = nullptr;

  if (parent != nullptr)
    parent->ReleaseChildCapture(this);
  else
    DisableCapture();
}

void
ContainerWindow::ClearCapture()
{
  Window::ClearCapture();

  if (capture_child != nullptr) {
    capture_child->ClearCapture();
    capture_child = nullptr;
  }
}
