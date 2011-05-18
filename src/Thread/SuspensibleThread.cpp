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

#include "Thread/SuspensibleThread.hpp"

#include <assert.h>

#ifndef HAVE_POSIX
SuspensibleThread::SuspensibleThread()
  :command_trigger(false), stop_trigger(true), suspend_trigger(true),
   suspended(true)
{}
#endif

bool
SuspensibleThread::Start()
{
#ifdef HAVE_POSIX
  stop_received = false;
  suspend_received = false;
  suspended = false;
#else
  suspend_trigger.Reset();
  stop_trigger.Reset();
  command_trigger.Reset();
  suspended.Reset();
#endif

  return Thread::Start();
}

void
SuspensibleThread::BeginStop()
{
#ifdef HAVE_POSIX
  mutex.Lock();
  stop_received = true;
  command_trigger.Signal();
  mutex.Unlock();
#else
  stop_trigger.Signal();
  command_trigger.Signal();
#endif
}

void
SuspensibleThread::BeginSuspend()
{
  assert(!Thread::IsInside());

#ifdef HAVE_POSIX
  mutex.Lock();
  suspend_received = true;
  command_trigger.Signal();
  mutex.Unlock();
#else
  suspend_trigger.Signal();
  command_trigger.Signal();
#endif
}

void
SuspensibleThread::WaitUntilSuspended()
{
  assert(!Thread::IsInside());

#ifdef HAVE_POSIX
  mutex.Lock();
  assert(suspend_received);

  while (!suspended)
    client_trigger.Wait(mutex);
  mutex.Unlock();
#else
  assert(suspend_trigger.Test());

  suspended.Wait();
#endif
}

void
SuspensibleThread::Suspend()
{
  BeginSuspend();
  WaitUntilSuspended();
}

void
SuspensibleThread::Resume()
{
#ifdef HAVE_POSIX
  mutex.Lock();
  suspend_received = false;
  command_trigger.Signal();
  mutex.Unlock();
#else
  suspend_trigger.Reset();
  command_trigger.Signal();
#endif
}

bool
SuspensibleThread::CheckStoppedOrSuspended()
{
  assert(Thread::IsInside());

#ifdef HAVE_POSIX
  mutex.Lock();

  assert(!suspended);

  if (!stop_received && suspend_received) {
    suspended = true;
    client_trigger.Signal();
    while (!stop_received && suspend_received)
      command_trigger.Wait(mutex);
    suspended = false;
  }

  bool stop = stop_received;
  mutex.Unlock();
  return stop;
#else
  if (stop_trigger.Test())
    return true;

  if (suspend_trigger.Test()) {
    suspended.Signal();
    while (suspend_trigger.Test() && !stop_trigger.Test())
      command_trigger.Wait();
    suspended.Reset();
  }

  return stop_trigger.Test();
#endif
}

bool
SuspensibleThread::WaitForStopped(unsigned timeout_ms)
{
  assert(Thread::IsInside());

#ifdef HAVE_POSIX
  mutex.Lock();

  assert(!suspended);
  suspended = true;

  if (!stop_received)
    command_trigger.Wait(mutex, timeout_ms);

  if (!stop_received && suspend_received) {
    client_trigger.Signal();
    while (!stop_received && suspend_received)
      command_trigger.Wait(mutex);
  }

  suspended = false;
  bool stop = stop_received;
  mutex.Unlock();
  return stop;
#else
  if (stop_trigger.Test())
    return true;

  suspended.Signal();

  command_trigger.Wait(timeout_ms);
  while (suspend_trigger.Test() && !stop_trigger.Test())
    command_trigger.Wait();

  suspended.Reset();

  return stop_trigger.Test();
#endif
}
