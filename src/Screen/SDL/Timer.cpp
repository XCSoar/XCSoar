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

#include "Screen/Timer.hpp"
#include "Screen/SDL/Event.hpp"
#include "Screen/Window.hpp"

void
Timer::Schedule(unsigned ms)
{
  Cancel();

  id = ::SDL_AddTimer(ms, Callback, this);
}

void
Timer::Cancel()
{
  if (!IsActive())
    return;

  ::SDL_RemoveTimer(id);
  id = NULL;

  EventQueue::Purge(Invoke, (void *)this);
  queued.store(false, std::memory_order_relaxed);
}

void
Timer::Invoke()
{
  OnTimer();
  queued.store(false, std::memory_order_relaxed);
}

void
Timer::Invoke(void *ctx)
{
  Timer *timer = (Timer *)ctx;
  timer->Invoke();
}

Uint32
Timer::Callback(Uint32 interval)
{
  if (!queued.exchange(true, std::memory_order_relaxed))
    EventQueue::Push(Invoke, (void *)this);
  return interval;
}

Uint32
Timer::Callback(Uint32 interval, void *param)
{
  Timer *timer = (Timer *)param;

  return timer->Callback(interval);
}

void
WindowTimer::OnTimer()
{
  window.OnTimer(*this);
}
