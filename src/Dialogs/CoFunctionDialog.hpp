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
                     const TCHAR *caption, Co::Task<T> task,
                     PluggableOperationEnvironment *env)
{
  ReturnValue<T> value;
  auto invoke = [&task, &value]() -> Co::InvokeTask {
    value.Set(co_await task);
  };

  if (ShowCoDialog(parent, dialog_look, caption, invoke(), env))
    return std::move(value).Get();

  return std::nullopt;
}
