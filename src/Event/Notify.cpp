/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Notify.hpp"

#ifdef ANDROID
#include "Event/Android/Queue.hpp"
#include "Android/Main.hpp"
#elif defined(USE_CONSOLE)
#include "Event/Console/Queue.hpp"
#include "Event/Console/Globals.hpp"
#elif defined(ENABLE_SDL)
#include "Event/SDL/Event.hpp"
#include "Event/SDL/Queue.hpp"
#endif

Notify::Notify()
  :pending(false)
{
#ifdef USE_GDI
  Window::CreateMessageWindow();
#endif
}

Notify::~Notify()
{
  if (pending.load(std::memory_order_relaxed)) {
#ifdef ANDROID
    event_queue->Purge(Callback, this);
#elif defined(ENABLE_SDL)
    EventQueue::Purge(Callback, this);
#endif
  }
}

void
Notify::SendNotification()
{
  if (pending.exchange(true, std::memory_order_relaxed))
    return;

#if defined(ANDROID) || defined(USE_CONSOLE)
  event_queue->Push(Event(Callback, this));
#elif defined(ENABLE_SDL)
  EventQueue::Push(Callback, this);
#else
  SendUser(0);
#endif
}

void
Notify::ClearNotification()
{
  if (!pending.exchange(false, std::memory_order_relaxed))
    return;

#if defined(ANDROID) || defined(USE_CONSOLE)
  event_queue->Purge(Callback, this);
#elif defined(ENABLE_SDL)
  EventQueue::Purge(Callback, this);
#endif
}

void
Notify::RunNotification()
{
  if (pending.exchange(false, std::memory_order_relaxed))
    OnNotification();
}

void
Notify::Callback(void *ctx)
{
  Notify &notify = *(Notify *)ctx;
  notify.RunNotification();
}

#ifdef USE_GDI

bool
Notify::OnUser(unsigned id)
{
  RunNotification();
  return true;
}

#endif
