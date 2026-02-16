// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThreadedOperationEnvironment.hpp"

ThreadedOperationEnvironment::ThreadedOperationEnvironment(OperationEnvironment &_other) noexcept
  :other(_other)
{
}

void
ThreadedOperationEnvironment::Cancel() noexcept
{
  const std::lock_guard lock{mutex};
  if (cancel_flag)
    return;

  cancel_flag = true;
  cancel_cond.notify_one();

  if (cancel_handler)
    cancel_handler();
}

bool
ThreadedOperationEnvironment::IsCancelled() const noexcept
{
  const std::lock_guard lock{mutex};
  return cancel_flag;
}

void
ThreadedOperationEnvironment::SetCancelHandler(std::function<void()> handler) noexcept
{
  const std::lock_guard lock{mutex};
  cancel_handler = std::move(handler);

  if (cancel_handler && cancel_flag)
    cancel_handler();
}

void
ThreadedOperationEnvironment::Sleep(std::chrono::steady_clock::duration duration) noexcept
{
  std::unique_lock lock{mutex};
  if (!cancel_flag)
    cancel_cond.wait_for(lock, duration);
}

void
ThreadedOperationEnvironment::SetErrorMessage(const char *_error) noexcept
{
  {
    const std::lock_guard lock{mutex};
    data.SetErrorMessage(_error);
  }

  notify.SendNotification();
}

void
ThreadedOperationEnvironment::SetText(const char *_text) noexcept
{
  {
    const std::lock_guard lock{mutex};
    data.SetText(_text);
  }

  notify.SendNotification();
}

void
ThreadedOperationEnvironment::SetProgressRange(unsigned range) noexcept
{
  if (LockSetProgressRange(range))
    notify.SendNotification();
}

void
ThreadedOperationEnvironment::SetProgressPosition(unsigned position) noexcept
{
  if (LockSetProgressPosition(position))
    notify.SendNotification();
}

void
ThreadedOperationEnvironment::OnNotification()
{
  const Data new_data = LockReceiveData();

  /* forward the method calls to the other OperationEnvironment */

  if (new_data.update_error)
    other.SetErrorMessage(new_data.error);

  if (new_data.update_text)
    other.SetText(new_data.text);

  if (new_data.update_progress_range)
    other.SetProgressRange(new_data.progress_range);

  if (new_data.update_progress_position)
    other.SetProgressPosition(new_data.progress_position);
}
