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

#ifndef XCSOAR_EVENT_TIMER_HPP
#define XCSOAR_EVENT_TIMER_HPP

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

#endif
