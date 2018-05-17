/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

ThreadedOperationEnvironment::ThreadedOperationEnvironment(OperationEnvironment &_other)
  :DelayedNotify(250), other(_other)
{
}

bool
ThreadedOperationEnvironment::IsCancelled() const
{
  const ScopeLock lock(mutex);
  return cancel_flag;
}

void
ThreadedOperationEnvironment::Sleep(unsigned ms)
{
  const ScopeLock lock(mutex);
  if (!cancel_flag)
    cancel_cond.timed_wait(mutex, ms);
}

void
ThreadedOperationEnvironment::SetErrorMessage(const TCHAR *_error)
{
  {
    const ScopeLock lock(mutex);
    data.SetErrorMessage(_error);
  }

  SendNotification();
}

void
ThreadedOperationEnvironment::SetText(const TCHAR *_text)
{
  {
    const ScopeLock lock(mutex);
    data.SetText(_text);
  }

  SendNotification();
}

void
ThreadedOperationEnvironment::SetProgressRange(unsigned range)
{
  if (LockSetProgressRange(range))
    SendNotification();
}

void
ThreadedOperationEnvironment::SetProgressPosition(unsigned position)
{
  if (LockSetProgressPosition(position))
    SendNotification();
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
