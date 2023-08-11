// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Notify.hpp"
#include "co/InjectTask.hxx"
#include "util/ReturnValue.hxx"

namespace UI {

/**
 * Execute a coroutine in the I/O thread and pass the return value to
 * a function in the UI thread.
 *
 * @param T the type of the coroutine's return value
 */
template<typename T>
class CoInjectFunction final {
  Co::InjectTask inject_task;

  ReturnValue<T> value;

  std::exception_ptr error;

  std::function<void(T)> on_success;
  std::function<void(std::exception_ptr)> on_error;

  Notify notify{[this]{
    if (error)
      on_error(std::move(error));
    else
      on_success(std::move(value).Get());
  }};

public:
  explicit CoInjectFunction(EventLoop &event_loop) noexcept
    :inject_task(event_loop) {}

  template<typename Task>
  void Start(Task &&task, std::function<void(T)> &&_on_success,
             std::function<void(std::exception_ptr)> &&_on_error) noexcept {
    on_success = std::move(_on_success);
    on_error = std::move(_on_error);

    inject_task.Start(Run(std::move(task)), BIND_THIS_METHOD(OnCompletion));
  }

  void Cancel() noexcept {
    inject_task.Cancel();
    notify.ClearNotification();
  }

private:
  template<typename Task>
  Co::InvokeTask Run(Task task) noexcept {
    value.Set(co_await task);
  }

  void OnCompletion(std::exception_ptr _error) noexcept {
    /* this runs in the I/O thread - move to the UI thread */
    error = std::move(_error);
    notify.SendNotification();
  }
};

} // namespace UI
