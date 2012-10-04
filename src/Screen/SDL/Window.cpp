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

#include "Screen/Window.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Debug.hpp"

#ifdef ANDROID
#include "Screen/Android/Event.hpp"
#include "Android/Main.hpp"
#else
#include "Screen/SDL/Event.hpp"
#endif

void
Window::Create(ContainerWindow *parent,
               PixelScalar left, PixelScalar top,
               UPixelScalar width, UPixelScalar height,
               const WindowStyle window_style)
{
  assert(IsScreenInitialized());
  assert(width < 0x8000);
  assert(height < 0x8000);

  double_clicks = window_style.double_clicks;

  this->parent = parent;
  this->left = left;
  this->top = top;
  this->width = width;
  this->height = height;

  tab_stop = window_style.tab_stop;
  control_parent = window_style.control_parent;
  visible = window_style.visible;
  enabled = window_style.enabled;
  has_border = window_style.has_border;
  text_style = window_style.text_style;

  if (parent != NULL)
    parent->AddChild(*this);

  OnCreate();
  OnResize(width, height);
}

void
Window::ToScreen(PixelRect &rc) const
{
  for (const Window *p = parent; p != NULL; p = p->parent) {
    rc.left += p->left;
    rc.top += p->top;
    rc.right += p->left;
    rc.bottom += p->top;
  }
}

PixelRect
Window::GetParentClientRect() const
{
  assert(parent != NULL);

  return parent->GetClientRect();
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
  return focused ? this : NULL;
}

void
Window::SetFocus()
{
  if (parent != NULL)
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
  assert(parent != NULL);

  parent->SetFocus();
}

void
Window::SetCapture()
{
  AssertNoneLocked();
  AssertThread();

  if (parent != NULL)
    parent->SetChildCapture(this);

  capture = true;
}

void
Window::ReleaseCapture()
{
  AssertNoneLocked();
  AssertThread();

  capture = false;

  if (parent != NULL)
    parent->ReleaseChildCapture(this);
}

void
Window::ClearCapture()
{
  capture = false;
}

void
Window::Setup(Canvas &canvas)
{
  if (font != NULL)
    canvas.Select(*font);
}

void
Window::Invalidate()
{
  if (visible && parent != NULL)
    parent->Invalidate();
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
  AssertNoneLocked();
  AssertThread();

  parent->BringChildToTop(*this);
}

void
Window::BringToBottom()
{
  AssertNoneLocked();
  AssertThread();

  parent->BringChildToBottom(*this);
}

void
Window::SendUser(unsigned id)
{
#ifdef ANDROID
  event_queue->Push(Event(Event::USER, id, this));
#else
  SDL_Event event;
  event.user.type = EVENT_USER;
  event.user.code = (int)id;
  event.user.data1 = this;
  event.user.data2 = NULL;

  ::SDL_PushEvent(&event);
#endif
}
