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

#include "Queue.hpp"
#include "OS/Clock.hpp"
#include "Util/Macros.hpp"

#include <poll.h>

EventQueue::EventQueue():running(true)
{
  event_pipe.Create();
}

void
EventQueue::Push(const Event &event)
{
  ScopeLock protect(mutex);
  if (!running)
    return;

  events.push(event);
  WakeUp();
}

int
EventQueue::GetTimeout() const
{
  auto i = timers.begin();
  if (i != timers.end()) {
    int64_t relative = i->due_us - MonotonicClockUS();
    if (relative <= 0)
      return 0;
    return relative / 1000 + 1;
  }

  return -1;
}

void
EventQueue::Poll()
{
  pollfd pfds[1] = {
    { event_pipe.GetReadFD(), POLLIN },
  };

  poll(pfds, ARRAY_SIZE(pfds), GetTimeout());
  event_pipe.Read();
}

void
EventQueue::Fill()
{
}

bool
EventQueue::Generate(Event &event)
{
  auto t = timers.begin();
  if (t != timers.end() && t->IsDue(MonotonicClockUS())) {
    event.type = Event::TIMER;
    event.ptr = t->timer;
    timers.erase(t);
    return true;
  }

  return false;
}

bool
EventQueue::Pop(Event &event)
{
  ScopeLock protect(mutex);
  if (!running || events.empty())
    return false;

  if (events.empty()) {
    if (Generate(event))
      return true;

    Fill();
  }

  event = events.front();
  events.pop();

  if (event.type == Event::QUIT)
    Quit();

  return true;
}

bool
EventQueue::Wait(Event &event)
{
  ScopeLock protect(mutex);
  if (!running)
    return false;

  if (events.empty()) {
    if (Generate(event))
      return true;

    Fill();
    while (events.empty()) {
      mutex.Unlock();
      Poll();
      mutex.Lock();

      if (Generate(event))
        return true;

      Fill();
    }
  }

  event = events.front();
  events.pop();

  if (event.type == Event::QUIT)
    Quit();

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

  timers.insert(TimerRecord(timer, MonotonicClockUS() + ms * 1000));
  WakeUp();
}

void
EventQueue::CancelTimer(Timer &timer)
{
  ScopeLock protect(mutex);

  for (auto i = timers.begin(), end = timers.end(); i != end; ++i) {
    if (i->timer == &timer) {
      timers.erase(i);
      return;
    }
  }
}
