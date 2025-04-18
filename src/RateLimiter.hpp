// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/event/Timer.hpp"
#include "time/PeriodClock.hpp"

/**
 * A class that limits the rate at which events are processed.  It
 * postpones processing for a certain amount of time, and combines
 * several events.
 *
 * Due to its use of timers and the event queue, this class can only
 * be used in the main thread.
 */
class RateLimiter {
  UI::Timer timer{[this]{ OnTimer(); }};

  /**
   * Remember the last Run() invocation.
   */
  PeriodClock clock;

  std::chrono::steady_clock::duration period, delay;

public:
  /**
   * Constructor.
   *
   * @param period_ms the minimum duration between two invocations
   * @param delay_ms an event is delayed by this duration to combine
   * consecutive invocations
   */
  RateLimiter(std::chrono::steady_clock::duration _period,
              std::chrono::steady_clock::duration _delay={}) noexcept;

  void Trigger();

  void Cancel() noexcept {
    timer.Cancel();
  }

protected:
  virtual void Run() = 0;

private:
  void OnTimer();
};
