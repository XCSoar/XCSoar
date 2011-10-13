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
Window::set(ContainerWindow *parent,
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

  tab_stop = window_style.m_tab_stop;
  control_parent = window_style.m_control_parent;
  visible = window_style.visible;
  enabled = window_style.enabled;
  has_border = window_style.has_border;
  text_style = window_style.text_style;

  if (parent != NULL)
    parent->add_child(*this);

  on_create();
  on_resize(width, height);
}

void
Window::to_screen(PixelRect &rc) const
{
  for (const Window *p = parent; p != NULL; p = p->parent) {
    rc.left += p->left;
    rc.top += p->top;
    rc.right += p->left;
    rc.bottom += p->top;
  }
}

Window *
Window::get_focused_window()
{
  return focused ? this : NULL;
}

void
Window::set_focus()
{
  if (parent != NULL)
    parent->set_active_child(*this);

  if (focused)
    return;

  on_setfocus();
}

void
Window::ClearFocus()
{
  if (focused) {
    on_killfocus();

    assert(!focused);
  }
}

void
Window::set_capture()
{
  assert_none_locked();
  assert_thread();

  if (parent != NULL)
    parent->set_child_capture(this);

  capture = true;
}

void
Window::release_capture()
{
  assert_none_locked();
  assert_thread();

  capture = false;

  if (parent != NULL)
    parent->release_child_capture(this);
}

void
Window::clear_capture()
{
  capture = false;
}

void
Window::setup(Canvas &canvas)
{
  if (font != NULL)
    canvas.select(*font);
}

void
Window::invalidate()
{
  if (visible && parent != NULL)
    parent->invalidate();
}

void
Window::expose()
{
  if (!visible)
    return;

  if (parent != NULL)
    parent->expose();
}

void
Window::show()
{
  assert_thread();

  if (visible)
    return;

  visible = true;
  parent->invalidate();
}

void
Window::hide()
{
  assert_thread();

  if (!visible)
    return;

  visible = false;
  parent->invalidate();
}

void
Window::bring_to_top()
{
  assert_none_locked();
  assert_thread();

  parent->bring_child_to_top(*this);
}

void
Window::BringToBottom()
{
  assert_none_locked();
  assert_thread();

  parent->BringChildToBottom(*this);
}

void
Window::send_user(unsigned id)
{
#ifdef ANDROID
  event_queue->push(Event(Event::USER, id, this));
#else
  SDL_Event event;
  event.user.type = EVENT_USER;
  event.user.code = (int)id;
  event.user.data1 = this;
  event.user.data2 = NULL;

  ::SDL_PushEvent(&event);
#endif
}

#ifndef ANDROID

void
Window::send_timer(SDLTimer *timer)
{
  SDL_Event event;
  event.user.type = EVENT_TIMER;
  event.user.code = 0;
  event.user.data1 = this;
  event.user.data2 = timer;

  ::SDL_PushEvent(&event);
}

#endif /* !ANDROID */
