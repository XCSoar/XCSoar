// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "../shared/TimerQueue.hpp"
#include "thread/Mutex.hxx"
#include "time/ClockCache.hxx"

#include <chrono>

#include <handleapi.h>

namespace UI {

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

} // namespace UI
