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

#include "Notify.hpp"
#include "Globals.hpp"
#include "Queue.hpp"

Notify::Notify()
  :pending(false)
{
#ifdef USE_WINUSER
  Window::CreateMessageWindow();
#endif
}

void
Notify::SendNotification()
{
  if (pending.exchange(true, std::memory_order_relaxed))
    return;

#if defined(ANDROID) || defined(USE_POLL_EVENT) || defined(ENABLE_SDL)
  event_queue->Push(Callback, this);
#else
  SendUser(0);
#endif
}

void
Notify::ClearNotification()
{
  if (!pending.exchange(false, std::memory_order_relaxed))
    return;

#if defined(ANDROID) || defined(USE_POLL_EVENT) || defined(ENABLE_SDL)
  event_queue->Purge(Callback, this);
#endif
}

void
Notify::RunNotification()
{
  if (pending.exchange(false, std::memory_order_relaxed))
    OnNotification();
}

#ifdef USE_WINUSER

bool
Notify::OnUser(unsigned id)
{
  RunNotification();
  return true;
}

#else

void
Notify::Callback(void *ctx)
{
  Notify &notify = *(Notify *)ctx;
  notify.RunNotification();
}

#endif
