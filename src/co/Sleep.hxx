// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Compat.hxx"
#include "event/CoarseTimerEvent.hxx"
#include "event/FineTimerEvent.hxx"
#include "event/Chrono.hxx"
#include "util/BindMethod.hxx"

#include <coroutine>

class EventLoop;

namespace Co {

/**
 * Suspend the current coroutine until @p delay has elapsed on
 * @p loop.  Must run on the thread that owns @p loop.
 */
template<typename Timer>
class BasicSleepAwaitable final {
  const Event::Duration delay;
  std::coroutine_handle<> continuation{nullptr};
  Timer timer;

  void OnExpired() noexcept {
    timer.Cancel();
    if (continuation)
      continuation.resume();
  }

public:
  BasicSleepAwaitable(EventLoop &loop, Event::Duration delay) noexcept
    :delay(delay),
     timer(loop, BIND_THIS_METHOD(OnExpired)) {}

  ~BasicSleepAwaitable() noexcept {
    timer.Cancel();
  }

  bool await_ready() const noexcept {
    return delay <= Event::Duration::zero();
  }

  void await_suspend(std::coroutine_handle<> h) noexcept {
    continuation = h;
    timer.Schedule(delay);
  }

  void await_resume() noexcept {}
};

using SleepAwaitable = BasicSleepAwaitable<CoarseTimerEvent>;

/**
 * Like #SleepAwaitable, but for delays below one second.
 */
using FineSleepAwaitable = BasicSleepAwaitable<FineTimerEvent>;

inline SleepAwaitable
Sleep(EventLoop &loop, Event::Duration delay) noexcept
{
  return SleepAwaitable{loop, delay};
}

inline FineSleepAwaitable
FineSleep(EventLoop &loop, Event::Duration delay) noexcept
{
  return FineSleepAwaitable{loop, delay};
}

} // namespace Co
