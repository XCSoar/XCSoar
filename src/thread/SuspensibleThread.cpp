// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "thread/SuspensibleThread.hpp"

#include <cassert>

void
SuspensibleThread::Start(bool _suspended)
{
  stop_received = false;
  suspend_received = _suspended;
  suspended = false;

  Thread::Start();
}

void
SuspensibleThread::BeginStop() noexcept
{
  assert(!Thread::IsInside());

  const std::lock_guard lock{mutex};
  _BeginStop();
}

void
SuspensibleThread::_BeginStop() noexcept
{
  assert(!Thread::IsInside());

  stop_received = true;
  command_trigger.notify_one();
}

void
SuspensibleThread::BeginSuspend() noexcept
{
  assert(!Thread::IsInside());
  assert(Thread::IsDefined());

  const std::lock_guard lock{mutex};
  _BeginSuspend();
}

void
SuspensibleThread::_BeginSuspend() noexcept
{
  assert(!Thread::IsInside());
  assert(Thread::IsDefined());

  suspend_received = true;
  command_trigger.notify_one();
}

void
SuspensibleThread::WaitUntilSuspended() noexcept
{
  assert(!Thread::IsInside());
  assert(Thread::IsDefined());

  std::unique_lock lock{mutex};
  _WaitUntilSuspended(lock);
}

void
SuspensibleThread::_WaitUntilSuspended(std::unique_lock<Mutex> &lock) noexcept
{
  assert(!Thread::IsInside());
  assert(Thread::IsDefined());
  assert(suspend_received);

  while (!suspended)
    client_trigger.wait(lock);
}

void
SuspensibleThread::Suspend() noexcept
{
  assert(!Thread::IsInside());

  BeginSuspend();
  WaitUntilSuspended();
}

void
SuspensibleThread::Resume() noexcept
{
  assert(!Thread::IsInside());

  const std::lock_guard lock{mutex};
  suspend_received = false;
  command_trigger.notify_one();
}

bool
SuspensibleThread::_IsCommandPending() const noexcept
{
  assert(Thread::IsInside());

  return stop_received || suspend_received;
}

bool
SuspensibleThread::IsCommandPending() noexcept
{
  assert(Thread::IsInside());

  const std::lock_guard lock{mutex};
  return _IsCommandPending();
}

bool
SuspensibleThread::_CheckStoppedOrSuspended(std::unique_lock<Mutex> &lock) noexcept
{
  assert(Thread::IsInside());

  assert(!suspended);

  if (!stop_received && suspend_received) {
    suspended = true;
    client_trigger.notify_one();
    while (!stop_received && suspend_received)
      command_trigger.wait(lock);
    suspended = false;
  }

  return stop_received;
}

bool
SuspensibleThread::CheckStoppedOrSuspended() noexcept
{
  assert(Thread::IsInside());

  std::unique_lock lock{mutex};
  return _CheckStoppedOrSuspended(lock);
}

bool
SuspensibleThread::_WaitForStopped(std::unique_lock<Mutex> &lock,
                                   std::chrono::steady_clock::duration timeout) noexcept
{
  assert(Thread::IsInside());

  assert(!suspended);
  suspended = true;

  if (!stop_received)
    command_trigger.wait_for(lock, timeout);

  if (!stop_received && suspend_received) {
    client_trigger.notify_one();
    while (!stop_received && suspend_received)
      command_trigger.wait(lock);
  }

  suspended = false;
  return stop_received;
}

bool
SuspensibleThread::WaitForStopped(std::chrono::steady_clock::duration timeout) noexcept
{
  assert(Thread::IsInside());

  std::unique_lock lock{mutex};
  return _WaitForStopped(lock, timeout);
}
