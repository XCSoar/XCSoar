// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/InjectTask.hxx"

class EventLoop;

namespace Net {

/**
 * Runs one coroutine on the network #EventLoop.
 *
 * Long-lived components (NOTAM, TIM, LiveTrack24, …) use this type and
 * call BeginShutdown() from NetComponents::BeginShutdown() before the
 * UI or parent object is destroyed.  Modal UI (ShowCoDialog) uses
 * Co::InjectTask directly and cancels when the dialog closes.
 */
class AsyncTask final {
  Co::InjectTask task;
  bool shutting_down = false;

public:
  explicit AsyncTask(EventLoop &event_loop) noexcept
    :task(event_loop) {}

  ~AsyncTask() noexcept {
    BeginShutdown();
  }

  AsyncTask(const AsyncTask &) = delete;
  AsyncTask &operator=(const AsyncTask &) = delete;

  [[nodiscard]]
  bool IsShuttingDown() const noexcept {
    return shutting_down;
  }

  [[nodiscard]]
  bool IsRunning() const noexcept {
    return !shutting_down && task;
  }

  void BeginShutdown() noexcept {
    if (shutting_down)
      return;

    shutting_down = true;
    task.Cancel();
  }

  void Start(Co::InvokeTask invoke_task,
             Co::InvokeTask::Callback callback) noexcept {
    if (shutting_down)
      return;

    task.Start(std::move(invoke_task), std::move(callback));
  }

  void Cancel() noexcept {
    task.Cancel();
  }
};

} // namespace Net
