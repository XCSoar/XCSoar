// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "CoDialog.hpp"
#include "co/Task.hxx"
#include "co/InvokeTask.hxx"
#include "util/ReturnValue.hxx"

/**
 * Like ShowCoDialog(), but accepts a Co::Task and returns the return
 * value of the coroutine (or std::nullopt if the user has canceled
 * the task).
 */
template<typename T>
[[nodiscard]]
std::optional<T>
ShowCoFunctionDialog(UI::SingleWindow &parent, const DialogLook &dialog_look,
                     const char *caption, Co::Task<T> &&task,
                     PluggableOperationEnvironment *env)
{
  ReturnValue<T> value;

  /* note: we pass the task as parameter by value so ownership gets
     transferred to the Co::InvokeTask instance, so the task gets
     destructed inside the I/O thread and not in this function
     (i.e. the UI thread) */
  auto invoke = [&value](Co::Task<T> task) -> Co::InvokeTask {
    value.Set(co_await task);
  };

  if (ShowCoDialog(parent, dialog_look, caption,
                   invoke(std::move(task)), env))
    return std::move(value).Get();

  return std::nullopt;
}
