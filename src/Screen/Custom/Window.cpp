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

#include "Screen/Window.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Debug.hpp"
#include "Screen/Canvas.hpp"

void
Window::Create(ContainerWindow *parent, PixelRect rc,
               const WindowStyle window_style)
{
  assert(IsScreenInitialized());
  assert(rc.left <= rc.right);
  assert(rc.GetWidth() < 0x8000);
  assert(rc.top <= rc.bottom);
  assert(rc.GetHeight() < 0x8000);

  this->parent = parent;
  position = rc.GetOrigin();
  size = rc.GetSize();

  tab_stop = window_style.tab_stop;
  control_parent = window_style.control_parent;
  visible = window_style.visible;
  enabled = window_style.enabled;
  has_border = window_style.has_border;

  if (parent != nullptr)
    parent->AddChild(*this);

  OnCreate();
  OnResize(size);
}

void
Window::ToScreen(PixelRect &rc) const
{
  assert(IsDefined());

  for (const Window *p = parent; p != nullptr; p = p->parent) {
    rc.left += p->position.x;
    rc.top += p->position.y;
    rc.right += p->position.x;
    rc.bottom += p->position.y;
  }
}

PixelRect
Window::GetParentClientRect() const
{
  assert(IsDefined());
  assert(parent != nullptr);

  return parent->GetClientRect();
}

bool
Window::IsMaximised() const
{
  assert(IsDefined());

  return parent != nullptr &&
    GetWidth() >= parent->GetWidth() && GetHeight() >= parent->GetHeight();
}

void
Window::SetEnabled(bool enabled)
{
  AssertThread();

  if (enabled == this->enabled)
    return;

  if (!enabled)
    /* cancel dragging before disabling this Window */
    OnCancelMode();

  this->enabled = enabled;
  Invalidate();
}

Window *
Window::GetFocusedWindow()
{
  assert(IsDefined());

  return focused ? this : nullptr;
}

void
Window::SetFocus()
{
  assert(IsDefined());

  if (!IsEnabled())
    return;

  if (parent != nullptr)
    parent->SetActiveChild(*this);

  if (focused)
    return;

  OnSetFocus();
}

void
Window::ClearFocus()
{
  if (focused) {
    OnKillFocus();

    assert(!focused);
  }
}

void
Window::FocusParent()
{
  AssertThread();
  assert(parent != nullptr);

  parent->SetFocus();
}

void
Window::SetCapture()
{
  AssertThread();

  if (parent != nullptr)
    parent->SetChildCapture(this);
  else
    EnableCapture();

  capture = true;
}

void
Window::ReleaseCapture()
{
  AssertThread();

  capture = false;

  if (parent != nullptr)
    parent->ReleaseChildCapture(this);
  else
    DisableCapture();
}

void
Window::ClearCapture()
{
  capture = false;
}

void
Window::Invalidate()
{
  AssertThread();
  assert(IsDefined());

  if (visible && parent != nullptr)
    parent->InvalidateChild(*this);
}

void
Window::Show()
{
  AssertThread();

  if (visible)
    return;

  visible = true;
  parent->Invalidate();
}

void
Window::Hide()
{
  AssertThread();

  if (!visible)
    return;

  visible = false;
  parent->Invalidate();
}

void
Window::BringToTop()
{
  AssertThread();

  parent->BringChildToTop(*this);
}

void
Window::BringToBottom()
{
  AssertThread();

  parent->BringChildToBottom(*this);
}
