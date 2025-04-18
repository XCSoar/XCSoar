// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_POLL_EVENT
#include "event/FineTimerEvent.hxx"
#endif

#include <chrono>
#include <functional>

namespace UI {

/**
 * A timer that calls a given function after a specified amount of
 * time.
 *
 * Initially, this class does not schedule a timer.
 *
 * This class is not thread safe; all of the methods must be called
 * from the main thread.
 */
class Timer final {
#ifdef USE_POLL_EVENT
  FineTimerEvent timer_event;
#else
  bool pending = false;
#endif

  using Callback = std::function<void()>;
  const Callback callback;

public:
  /**
   * Construct a Timer object that is not set initially.
   */
#ifdef USE_POLL_EVENT
  explicit Timer(Callback _callback) noexcept;
#else
  explicit Timer(Callback &&_callback) noexcept:callback(std::move(_callback)) {}
#endif

  Timer(const Timer &other) = delete;

  ~Timer() {
    Cancel();
  }

  /**
   * Is the timer pending?
   */
  bool IsPending() const noexcept {
#ifdef USE_POLL_EVENT
    return timer_event.IsPending();
#else
    return pending;
#endif
  }

  /**
   * Schedule the timer.  Cancels the previous setting if there was
   * one.
   */
  void Schedule(std::chrono::steady_clock::duration d) noexcept;

  /**
   * Schedule the timer.  Preserves the previous setting if there was
   * one.
   */
  void SchedulePreserve(std::chrono::steady_clock::duration d) noexcept;

  /**
   * Cancels the scheduled timer, if any.  This is safe to be called
   * while the timer is running.
   */
  void Cancel();

#ifdef USE_POLL_EVENT
private:
  void OnTimer() noexcept;

#else
public:
  void Invoke();
#endif
};

} // namespace UI
