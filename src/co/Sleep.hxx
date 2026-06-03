// SPDX-License-Identifier: BSD-2-Clause
// Copyright The XCSoar Project

#pragma once

#include "Compat.hxx"
#include "event/CoarseTimerEvent.hxx"
#include "event/Chrono.hxx"
#include "util/BindMethod.hxx"

#include <coroutine>

class EventLoop;

namespace Co {

/**
 * Suspend the current coroutine until @p delay has elapsed on
 * @p loop.  Must run on the thread that owns @p loop.
 */
class SleepAwaitable final {
  EventLoop &loop;
  const Event::Duration delay;
  std::coroutine_handle<> continuation{nullptr};
  CoarseTimerEvent timer;

  void OnExpired() noexcept {
    timer.Cancel();
    if (continuation)
      continuation.resume();
  }

public:
  SleepAwaitable(EventLoop &loop, Event::Duration delay) noexcept
    :loop(loop), delay(delay),
     timer(loop, BIND_THIS_METHOD(OnExpired)) {}

  ~SleepAwaitable() noexcept {
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

inline SleepAwaitable
Sleep(EventLoop &loop, Event::Duration delay) noexcept
{
  return SleepAwaitable{loop, delay};
}

} // namespace Co
