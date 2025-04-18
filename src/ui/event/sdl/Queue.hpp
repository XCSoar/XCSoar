// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
