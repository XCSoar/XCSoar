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

#include "Screen/Android/Event.hpp"
#include "Screen/TopWindow.hpp"
#include "Thread/Notify.hpp"
#include "Android/Timer.hpp"

void
EventQueue::Push(const Event &event)
{
  ScopeLock protect(mutex);
  if (!running)
    return;

  events.push(event);
  cond.Signal();
}

bool
EventQueue::Pop(Event &event)
{
  ScopeLock protect(mutex);
  if (!running || events.empty())
    return false;

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

  while (events.empty())
    cond.Wait(mutex);

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
  const enum Event::type *type_p = (const enum Event::type *)ctx;
  return event.type == *type_p;
}

void
EventQueue::Purge(enum Event::type type)
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
match_notify(const Event &event, void *ctx)
{
  return event.type == Event::NOTIFY && event.ptr == ctx;
}

void
EventQueue::Purge(Notify &notify)
{
  Purge(match_notify, (void *)&notify);
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

static bool
match_timer(const Event &event, void *ctx)
{
  return event.type == Event::TIMER && event.ptr == ctx;
}

void
EventQueue::Purge(AndroidTimer &timer)
{
  Purge(match_timer, (void *)&timer);
}

bool
EventLoop::Get(Event &event)
{
  if (bulk) {
    if (queue.Pop(event))
      return event.type != Event::QUIT;

    /* that was the last event for now, refresh the screen now */
    top_window.Refresh();
    bulk = false;
  }

  if (queue.Wait(event)) {
    bulk = true;
    return event.type != Event::QUIT;
  }

  return false;
}

void
EventLoop::Dispatch(const Event &event)
{
  if (event.type == Event::USER) {
    Window *window = (Window *)event.ptr;
    window->OnUser(event.param);
  } else if (event.type == Event::TIMER) {
    AndroidTimer *timer = (AndroidTimer *)event.ptr;
    timer->run();
  } else if (event.type == Event::CALLBACK) {
    event.callback(event.ptr);
  } else if (event.type == Event::NOTIFY) {
    Notify *notify = (Notify *)event.ptr;
    notify->RunNotification();
  } else if (event.type != Event::NOP)
    top_window.OnEvent(event);
}
