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

#include "Queue.hpp"
#include "OS/Clock.hpp"

EventQueue::EventQueue()
 :now_us(MonotonicClockUS()),
  quit(false) {}

void
EventQueue::Push(const Event &event)
{
  ScopeLock protect(mutex);
  if (quit)
    return;

  events.push(event);
  cond.signal();
}

bool
EventQueue::Pop(Event &event)
{
  ScopeLock protect(mutex);
  if (quit || events.empty())
    return false;

  event = events.front();
  events.pop();
  return true;
}

bool
EventQueue::Generate(Event &event)
{
  Timer *timer = timers.Pop(now_us);
  if (timer != nullptr) {
    event.type = Event::TIMER;
    event.ptr = timer;
    return true;
  }

  return false;
}

bool
EventQueue::Wait(Event &event)
{
  ScopeLock protect(mutex);
  if (quit)
    return false;

  if (events.empty())
    now_us = MonotonicClockUS();

  while (events.empty()) {
    if (Generate(event))
      return true;

    const int64_t timeout_us = timers.GetTimeoutUS(now_us);
    if (timeout_us < 0)
      cond.wait(mutex);
    else
      cond.timed_wait(mutex, (timeout_us + 999) / 1000);

    now_us = MonotonicClockUS();
  }

  event = events.front();
  events.pop();
  return true;
}

void
EventQueue::Purge(bool (*match)(const Event &event, void *ctx), void *ctx)
{
  ScopeLock protect(mutex);
  size_t n = events.size();
  while (n-- > 0) {
    if (!match(events.front(), ctx))
      events.push(events.front());
    events.pop();
  }
}

static bool
match_type(const Event &event, void *ctx)
{
  const Event::Type *type_p = (const Event::Type *)ctx;
  return event.type == *type_p;
}

void
EventQueue::Purge(Event::Type type)
{
  Purge(match_type, &type);
}

static bool
MatchCallback(const Event &event, void *ctx)
{
  const Event *match = (const Event *)ctx;
  return event.type == Event::CALLBACK && event.callback == match->callback &&
    event.ptr == match->ptr;
}

void
EventQueue::Purge(Event::Callback callback, void *ctx)
{
  Event match(callback, ctx);
  Purge(MatchCallback, (void *)&match);
}

static bool
match_window(const Event &event, void *ctx)
{
  return event.type == Event::USER && event.ptr == ctx;
}

void
EventQueue::Purge(Window &window)
{
  Purge(match_window, (void *)&window);
}

void
EventQueue::AddTimer(Timer &timer, unsigned ms)
{
  ScopeLock protect(mutex);

  timers.Add(timer, MonotonicClockUS() + ms * 1000);
  cond.signal();
}

void
EventQueue::CancelTimer(Timer &timer)
{
  ScopeLock protect(mutex);

  timers.Cancel(timer);
}
