// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/InvokeTask.hxx"
#include "event/Loop.hxx"
#include "event/DeferEvent.hxx"

class CoInstance {
  EventLoop event_loop;

  Co::InvokeTask invoke_task;

  DeferEvent defer_start{event_loop, BIND_THIS_METHOD(OnDeferredStart)};

  std::exception_ptr error;

public:
  auto &GetEventLoop() noexcept {
    return event_loop;
  }

  void Run(Co::InvokeTask &&_task) {
    invoke_task = std::move(_task);
    defer_start.Schedule();
    event_loop.Run();
    if (error)
      std::rethrow_exception(error);
  }

private:
  void OnCompletion(std::exception_ptr _error) noexcept {
    error = std::move(_error);
    event_loop.Break();
  }

  void OnDeferredStart() noexcept {
    invoke_task.Start(BIND_THIS_METHOD(OnCompletion));
  }
};
