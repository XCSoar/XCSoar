// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Timer.hpp"

namespace UI {

/**
 * A timer that, once initialized, periodically calls a given function
 * after a specified amount of time, until Cancel() gets called.
 *
 * Initially, this class does not schedule a timer.
 *
 * This class is not thread safe; all of the methods must be called
 * from the main thread.
 */
class PeriodicTimer final {
  Timer timer{[this]{ Invoke(); }};

  std::chrono::steady_clock::duration interval{-1};

  using Callback = std::function<void()>;
  const Callback callback;

public:
  /**
   * Construct an inactive timer.  Activate it by calling Schedule().
   */
  explicit PeriodicTimer(Callback &&_callback) noexcept
    :callback(std::move(_callback)) {}

  /**
   * Is the timer active, i.e. is it waiting for the current period to
   * end?
   */
  bool IsActive() const {
    return interval.count() >= 0;
  }

  /**
   * Schedule the timer.  Cancels the previous setting if there was
   * one.
   */
  void Schedule(std::chrono::steady_clock::duration d) noexcept {
    interval = d;
    timer.Schedule(d);
  }

  /**
   * Cancels the scheduled timer, if any.  This is safe to be called
   * while the timer is running.
   */
  void Cancel() noexcept {
    interval = std::chrono::steady_clock::duration{-1};
    timer.Cancel();
  }

public:
  void Invoke() noexcept {
    callback();

    if (IsActive() && !timer.IsPending())
      timer.Schedule(interval);
  }
};

} // namespace UI
