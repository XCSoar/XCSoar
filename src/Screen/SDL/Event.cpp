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

#include "Screen/SDL/Event.hpp"
#include "Thread/Debug.hpp"
#include "Thread/Notify.hpp"
#include "Asset.hpp"

#include "Screen/TopWindow.hpp"

bool
EventLoop::get(SDL_Event &event)
{
  if (bulk) {
    if (::SDL_PollEvent(&event))
      return true;

    /* that was the last event for now, refresh the screen now */
    top_window.refresh();
    bulk = false;
  }

  if (::SDL_WaitEvent(&event)) {
    bulk = true;
    return true;
  }

  return false;
}

void
EventLoop::dispatch(SDL_Event &event)
{
  if (event.type == EVENT_USER && event.user.data1 != NULL) {
    Window *window = (Window *)event.user.data1;
    window->on_user(event.user.code);
  } else if (event.type == EVENT_TIMER && event.user.data1 != NULL) {
    Window *window = (Window *)event.user.data1;
    SDLTimer *timer = (SDLTimer *)event.user.data2;
    window->on_timer(timer);
  } else if (event.type == EVENT_NOTIFY && event.user.data1 != NULL) {
    Notify *notify = (Notify *)event.user.data1;
    notify->RunNotification();
  } else
    top_window.on_event(event);
}

void
EventQueue::purge(Uint32 mask,
                  bool (*match)(const SDL_Event &event, void *ctx),
                  void *ctx)
{
  SDL_Event events[256]; // is that enough?
  int count = SDL_PeepEvents(events, 256, SDL_GETEVENT, mask);
  assert(count >= 0);

  for (int i = count - 1; i >= 0; --i)
    if (!match(events[i], ctx))
      std::copy(events + i + 1, events + count--, events + i);
  SDL_PeepEvents(events, count, SDL_ADDEVENT, mask);
}

static bool
match_notify(const SDL_Event &event, void *ctx)
{
  return event.type == EVENT_NOTIFY && event.user.data1 == ctx;
}

void
EventQueue::purge(Notify &notify)
{
  purge(SDL_EVENTMASK(EVENT_NOTIFY), match_notify, (void *)&notify);
}

static bool
match_window(const SDL_Event &event, void *ctx)
{
  return (event.type == EVENT_USER ||
          event.type == EVENT_TIMER) &&
    event.user.data1 == ctx;
}

void
EventQueue::purge(Window &window)
{
  purge(SDL_EVENTMASK(EVENT_USER) | SDL_EVENTMASK(EVENT_TIMER),
        match_window, (void *)&window);
}

static bool
match_timer(const SDL_Event &event, void *ctx)
{
  return event.type == EVENT_TIMER && event.user.data2 == ctx;
}

void
EventQueue::purge(SDLTimer &timer)
{
  purge(SDL_EVENTMASK(EVENT_TIMER), match_timer, (void *)&timer);
}
