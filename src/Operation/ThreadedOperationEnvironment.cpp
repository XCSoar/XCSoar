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

#include "ThreadedOperationEnvironment.hpp"

ThreadedOperationEnvironment::ThreadedOperationEnvironment(OperationEnvironment &_other) noexcept
  :other(_other)
{
}

void
ThreadedOperationEnvironment::Cancel() noexcept
{
  const std::lock_guard<Mutex> lock(mutex);
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
  const std::lock_guard<Mutex> lock(mutex);
  return cancel_flag;
}

void
ThreadedOperationEnvironment::SetCancelHandler(std::function<void()> handler) noexcept
{
  const std::lock_guard<Mutex> lock(mutex);
  cancel_handler = std::move(handler);

  if (cancel_handler && cancel_flag)
    cancel_handler();
}

void
ThreadedOperationEnvironment::Sleep(std::chrono::steady_clock::duration duration) noexcept
{
  std::unique_lock<Mutex> lock(mutex);
  if (!cancel_flag)
    cancel_cond.wait_for(lock, duration);
}

void
ThreadedOperationEnvironment::SetErrorMessage(const TCHAR *_error) noexcept
{
  {
    const std::lock_guard<Mutex> lock(mutex);
    data.SetErrorMessage(_error);
  }

  notify.SendNotification();
}

void
ThreadedOperationEnvironment::SetText(const TCHAR *_text) noexcept
{
  {
    const std::lock_guard<Mutex> lock(mutex);
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
