/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "../shared/TimerQueue.hpp"
#include "thread/Mutex.hxx"
#include "time/ClockCache.hxx"

#include <SDL_events.h>

#include <chrono>

namespace UI {

class EventQueue {
  ClockCache<std::chrono::steady_clock> steady_clock_cache;

  Mutex mutex;
  TimerQueue timers;

  bool quit;

public:
  EventQueue() noexcept;

  void Suspend() noexcept {}
  void Resume() noexcept {}

  /**
   * Caching wrapper for std::chrono::steady_clock::now().  The
   * real clock is queried at most once per event loop
   * iteration, because it is assumed that the event loop runs
   * for a negligible duration.
   */
  [[gnu::pure]]
  const auto &SteadyNow() const noexcept {
    return steady_clock_cache.now();
  }

  bool IsQuit() const noexcept {
    return quit;
  }

  void Quit() noexcept {
    quit = true;
  }

  void InjectCall(EventLoop::Callback callback, void *ctx) noexcept;

private:
  bool Generate(Event &event) noexcept;

public:
  bool Pop(Event &event) noexcept;
  bool Wait(Event &event) noexcept;

  /**
   * Purge all matching events from the event queue.
   */
  void Purge(Uint32 event,
             bool (*match)(const SDL_Event &event, void *ctx) noexcept,
             void *ctx) noexcept;

  /**
   * Purge all events for this callback from the event queue.
   */
  void Purge(EventLoop::Callback callback, void *ctx) noexcept;

  void AddTimer(Timer &timer, std::chrono::steady_clock::duration d) noexcept;
  void CancelTimer(Timer &timer) noexcept;

private:
  void FlushClockCaches() noexcept {
    steady_clock_cache.flush();
  }
};

} // namespace UI

#endif
