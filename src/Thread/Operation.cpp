/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Thread/Operation.hpp"
#include "OS/Sleep.h"

ThreadedOperationEnvironment::ThreadedOperationEnvironment(OperationEnvironment &_other)
  :other(_other), cancelled(true)
{
}

bool
ThreadedOperationEnvironment::IsCancelled() const
{
  /* this const_cast is a hack: Trigger::Test() cannot be const,
     because Test() resets the value for automatic-reset events */
  return const_cast<Trigger &>(cancelled).Test();
}

void
ThreadedOperationEnvironment::Sleep(unsigned ms)
{
  cancelled.Wait(ms);
}

void
ThreadedOperationEnvironment::SetText(const TCHAR *_text)
{
  mutex.Lock();
  data.SetText(_text);
  mutex.Unlock();

  SendNotification();
}

void
ThreadedOperationEnvironment::SetProgressRange(unsigned range)
{
  mutex.Lock();
  bool modified = data.SetProgressRange(range);
  mutex.Unlock();

  if (modified)
    SendNotification();
}

void
ThreadedOperationEnvironment::SetProgressPosition(unsigned position)
{
  mutex.Lock();
  bool modified = data.SetProgressPosition(position);
  mutex.Unlock();

  if (modified)
    SendNotification();
}

void
ThreadedOperationEnvironment::OnNotification()
{
  mutex.Lock();
  Data new_data = data;
  data.ClearUpdate();
  mutex.Unlock();

  /* forward the method calls to the other OperationEnvironment */

  if (new_data.update_text)
    other.SetText(new_data.text);

  if (new_data.update_progress_range)
    other.SetProgressRange(new_data.progress_range);

  if (new_data.update_progress_position)
    other.SetProgressPosition(new_data.progress_position);
}
