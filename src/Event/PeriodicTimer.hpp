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

#ifndef XCSOAR_EVENT_PERIODIC_TIMER_HPP
#define XCSOAR_EVENT_PERIODIC_TIMER_HPP

#include "Timer.hpp"

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

#endif
