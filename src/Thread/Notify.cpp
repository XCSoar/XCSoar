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

#include "Thread/Notify.hpp"

#ifdef ANDROID
#include "Screen/Android/Event.hpp"
#include "Android/Main.hpp"
#elif defined(ENABLE_SDL)
#include "Screen/SDL/Event.hpp"
#else
#endif

Notify::Notify()
{
#ifndef ENABLE_SDL
  Window::CreateMessageWindow();
#endif
}

Notify::~Notify()
{
#ifdef ANDROID
  event_queue->purge(*this);
#elif defined(ENABLE_SDL)
  EventQueue::purge(*this);
#endif
}

void
Notify::SendNotification()
{
  if (pending.GetAndSet())
    return;

#ifdef ANDROID
  event_queue->push(Event(Event::NOTIFY, this));
#elif defined(ENABLE_SDL)
  SDL_Event event;
  event.type = EVENT_NOTIFY;
  event.user.data1 = this;
  ::SDL_PushEvent(&event);
#else
  send_user(0);
#endif
}

void
Notify::RunNotification()
{
  if (pending.GetAndClear())
    OnNotification();
}

#ifndef ENABLE_SDL

bool
Notify::on_user(unsigned id)
{
  RunNotification();
  return true;
}

#endif
