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

#ifndef XCSOAR_EVENT_ANDROID_QUEUE_HPP
#define XCSOAR_EVENT_ANDROID_QUEUE_HPP

#include "../Shared/TimerQueue.hpp"
#include "../Shared/Event.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hxx"

#include <queue>

class Window;

class EventQueue {
  std::queue<Event> events;

  /**
   * The current time after the event thread returned from sleeping.
   */
  uint64_t now_us;

  TimerQueue timers;

  Mutex mutex;
  Cond cond;

  bool quit;

public:
  EventQueue();

  EventQueue(const EventQueue &) = delete;

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

  void Push(const Event &event);

  void Push(Event::Callback callback, void *ctx) {
    Push(Event(callback, ctx));
  }

  bool Pop(Event &event);

  bool Wait(Event &event);

  void Purge(bool (*match)(const Event &event, void *ctx), void *ctx);

  void Purge(Event::Type type);
  void Purge(Event::Callback callback, void *ctx);
  void Purge(Window &window);

  void AddTimer(Timer &timer, unsigned ms);
  void CancelTimer(Timer &timer);

private:
  bool Generate(Event &event);
};

#endif
