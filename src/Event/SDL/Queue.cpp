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
#include "Event.hpp"
#include "../Timer.hpp"
#include "OS/Sleep.h"
#include "OS/Clock.hpp"

EventQueue::EventQueue()
  :now_us(MonotonicClockUS()),
   quit(false) {}

void
EventQueue::Push(EventLoop::Callback callback, void *ctx)
{
  SDL_Event event;
  event.type = EVENT_CALLBACK;
  event.user.data1 = (void *)callback;
  event.user.data2 = ctx;
  ::SDL_PushEvent(&event);
}

static void
InvokeTimer(void *ctx)
{
  Timer *timer = (Timer *)ctx;
  timer->Invoke();
}

bool
EventQueue::Generate(Event &event)
{
  Timer *timer = timers.Pop(now_us);
  if (timer != nullptr) {
    event.event.type = EVENT_CALLBACK;
    event.event.user.data1 = (void *)InvokeTimer;
    event.event.user.data2 = (void *)timer;
    return true;
  }

  return false;
}

bool
EventQueue::Pop(Event &event)
{
  return !quit && (Generate(event) || ::SDL_PollEvent(&event.event));
}

bool
EventQueue::Wait(Event &event)
{
  /* this busy loop is ugly, and I wish we could do better than that,
     but SDL_WaitEvent() is just as bad; however copying this busy
     loop allows us to plug in more event sources */

  if (quit)
    return false;

  while (true) {
    if (Generate(event))
      return true;

    ::SDL_PumpEvents();
    int result = ::SDL_PeepEvents(&event.event, 1,
                                 SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
    if (result != 0)
      return result > 0;

    Sleep(10);

    now_us = MonotonicClockUS();
  }
}

void
EventQueue::Purge(Uint32 event,
                  bool (*match)(const SDL_Event &event, void *ctx),
                  void *ctx)
{
  SDL_Event events[256]; // is that enough?
  int count = SDL_PeepEvents(events, 256, SDL_GETEVENT, event, event);
  assert(count >= 0);

  SDL_Event *dest = events;
  for (const SDL_Event *src = events, *end = src + count; src != end; ++src)
    if (!match(*src, ctx))
      *dest++ = *src;

  SDL_PeepEvents(events, dest - events, SDL_ADDEVENT, event, event);
}

struct MatchCallbackData {
  void *data1, *data2;
};

static bool
MatchCallback(const SDL_Event &event, void *ctx)
{
  const MatchCallbackData *data = (const MatchCallbackData *)ctx;
  return event.type == EVENT_CALLBACK && event.user.data1 == data->data1 &&
    event.user.data2 == data->data2;
}

void
EventQueue::Purge(EventLoop::Callback callback, void *ctx)
{
  MatchCallbackData data { (void *)callback, ctx };
  Purge(EVENT_CALLBACK, MatchCallback, (void *)&data);
}

static bool
match_window(const SDL_Event &event, void *ctx)
{
  return event.type == EVENT_USER &&
    event.user.data1 == ctx;
}

void
EventQueue::Purge(Window &window)
{
  Purge(EVENT_USER,
        match_window, (void *)&window);
}

void
EventQueue::AddTimer(Timer &timer, unsigned ms)
{
  ScopeLock protect(mutex);

  timers.Add(timer, MonotonicClockUS() + ms * 1000);
}

void
EventQueue::CancelTimer(Timer &timer)
{
  ScopeLock protect(mutex);

  timers.Cancel(timer);
}
