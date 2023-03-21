// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "../shared/TimerQueue.hpp"
#include "../shared/Event.hpp"
#include "thread/Mutex.hxx"
#include "thread/Cond.hxx"
#include "time/ClockCache.hxx"

#include <chrono>
#include <queue>

namespace UI {

class EventQueue {
  std::queue<Event> events;

  ClockCache<std::chrono::steady_clock> steady_clock_cache;

  TimerQueue timers;

  Mutex mutex;
  Cond cond;

  bool quit = false;

public:
  EventQueue() = default;

  EventQueue(const EventQueue &) = delete;

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

  void Inject(const Event &event) noexcept;

  void InjectCall(Event::Callback callback, void *ctx) noexcept {
    Inject(Event{callback, ctx});
  }

  bool Pop(Event &event) noexcept;

  bool Wait(Event &event) noexcept;

  void Purge(bool (*match)(const Event &event, void *ctx) noexcept,
             void *ctx) noexcept;

  void Purge(Event::Type type) noexcept;
  void Purge(Event::Callback callback, void *ctx) noexcept;

  void AddTimer(Timer &timer, std::chrono::steady_clock::duration d) noexcept;
  void CancelTimer(Timer &timer) noexcept;

private:
  void FlushClockCaches() noexcept {
    steady_clock_cache.flush();
  }

  bool Generate(Event &event) noexcept;
};

} // namespace UI
