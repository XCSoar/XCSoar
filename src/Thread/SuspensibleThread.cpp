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

#include "Thread/SuspensibleThread.hpp"

#include <assert.h>

bool
SuspensibleThread::Start(bool _suspended)
{
  stop_received = false;
  suspend_received = _suspended;
  suspended = false;

  return Thread::Start();
}

void
SuspensibleThread::BeginStop()
{
  assert(!Thread::IsInside());

  const ScopeLock lock(mutex);
  _BeginStop();
}

void
SuspensibleThread::_BeginStop()
{
  assert(!Thread::IsInside());

  stop_received = true;
  command_trigger.signal();
}

void
SuspensibleThread::BeginSuspend()
{
  assert(!Thread::IsInside());
  assert(Thread::IsDefined());

  const ScopeLock lock(mutex);
  _BeginSuspend();
}

void
SuspensibleThread::_BeginSuspend()
{
  assert(!Thread::IsInside());
  assert(Thread::IsDefined());

  suspend_received = true;
  command_trigger.signal();
}

void
SuspensibleThread::WaitUntilSuspended()
{
  assert(!Thread::IsInside());
  assert(Thread::IsDefined());

  const ScopeLock lock(mutex);
  _WaitUntilSuspended();
}

void
SuspensibleThread::_WaitUntilSuspended()
{
  assert(!Thread::IsInside());
  assert(Thread::IsDefined());
  assert(suspend_received);

  while (!suspended)
    client_trigger.wait(mutex);
}

void
SuspensibleThread::Suspend()
{
  assert(!Thread::IsInside());

  BeginSuspend();
  WaitUntilSuspended();
}

void
SuspensibleThread::Resume()
{
  assert(!Thread::IsInside());

  const ScopeLock lock(mutex);
  suspend_received = false;
  command_trigger.signal();
}

bool
SuspensibleThread::_IsCommandPending() const
{
  assert(Thread::IsInside());
  assert(mutex.IsLockedByCurrent());

  return stop_received || suspend_received;
}

bool
SuspensibleThread::IsCommandPending()
{
  assert(Thread::IsInside());

  const ScopeLock lock(mutex);
  return _IsCommandPending();
}

bool
SuspensibleThread::_CheckStoppedOrSuspended()
{
  assert(Thread::IsInside());
  assert(mutex.IsLockedByCurrent());

  assert(!suspended);

  if (!stop_received && suspend_received) {
    suspended = true;
    client_trigger.signal();
    while (!stop_received && suspend_received)
      command_trigger.wait(mutex);
    suspended = false;
  }

  return stop_received;
}

bool
SuspensibleThread::CheckStoppedOrSuspended()
{
  assert(Thread::IsInside());

  const ScopeLock lock(mutex);
  return _CheckStoppedOrSuspended();
}

bool
SuspensibleThread::_WaitForStopped(unsigned timeout_ms)
{
  assert(Thread::IsInside());
  assert(mutex.IsLockedByCurrent());

  assert(!suspended);
  suspended = true;

  if (!stop_received)
    command_trigger.timed_wait(mutex, timeout_ms);

  if (!stop_received && suspend_received) {
    client_trigger.signal();
    while (!stop_received && suspend_received)
      command_trigger.wait(mutex);
  }

  suspended = false;
  return stop_received;
}

bool
SuspensibleThread::WaitForStopped(unsigned timeout_ms)
{
  assert(Thread::IsInside());

  const ScopeLock lock(mutex);
  return _WaitForStopped(timeout_ms);
}
