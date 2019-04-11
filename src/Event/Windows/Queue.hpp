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

#ifndef XCSOAR_EVENT_WINDOWS_QUEUE_HPP
#define XCSOAR_EVENT_WINDOWS_QUEUE_HPP

#include "../Shared/TimerQueue.hpp"
#include "Thread/Mutex.hpp"
#include "Time/ClockCache.hxx"

#include <chrono>

#include <windows.h>

struct Event;

class EventQueue {
  ClockCache<std::chrono::steady_clock> steady_clock_cache;

  HANDLE trigger;

  Mutex mutex;
  TimerQueue timers;

public:
  EventQueue();

  ~EventQueue() {
    ::CloseHandle(trigger);
  }

  /**
   * Caching wrapper for std::chrono::steady_clock::now().  The
   * real clock is queried at most once per event loop
   * iteration, because it is assumed that the event loop runs
   * for a negligible duration.
   */
  gcc_pure
  const auto &SteadyNow() const noexcept {
    return steady_clock_cache.now();
  }

  bool Wait(Event &event);

private:
  void WakeUp() {
    ::SetEvent(trigger);
  }

public:
  void AddTimer(Timer &timer, std::chrono::steady_clock::duration d) noexcept;
  void CancelTimer(Timer &timer);

  /**
   * Handle all pending repaint messages.
   */
  static void HandlePaintMessages();

private:
  void FlushClockCaches() noexcept {
    steady_clock_cache.flush();
  }
};

#endif
