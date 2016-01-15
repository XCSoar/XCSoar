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

#ifndef XCSOAR_EVENT_SDL_QUEUE_HPP
#define XCSOAR_EVENT_SDL_QUEUE_HPP

#include "Loop.hpp"
#include "../Shared/TimerQueue.hpp"
#include "Thread/Mutex.hpp"

#include <SDL_events.h>

class Window;

class EventQueue {
  /**
   * The current time after the event thread returned from sleeping.
   */
  uint64_t now_us;

  Mutex mutex;
  TimerQueue timers;

  bool quit;

public:
  EventQueue();

  /**
   * Returns the monotonic clock in microseconds.  This method is only
   * available in the main thread.
   */
  gcc_pure
  uint64_t ClockUS() const {
    return now_us;
  }

  bool IsQuit() const {
    return quit;
  }

  void Quit() {
    quit = true;
  }

  void Push(EventLoop::Callback callback, void *ctx);

private:
  bool Generate(Event &event);

public:
  bool Pop(Event &event);
  bool Wait(Event &event);

  /**
   * Purge all matching events from the event queue.
   */
  void Purge(Uint32 event,
             bool (*match)(const SDL_Event &event, void *ctx), void *ctx);

  /**
   * Purge all events for this callback from the event queue.
   */
  void Purge(EventLoop::Callback callback, void *ctx);

  /**
   * Purge all events for this Window from the event queue.
   */
  void Purge(Window &window);

  void AddTimer(Timer &timer, unsigned ms);
  void CancelTimer(Timer &timer);
};

#endif
