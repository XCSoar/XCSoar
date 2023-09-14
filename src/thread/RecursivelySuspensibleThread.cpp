// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RecursivelySuspensibleThread.hpp"

#include <cassert>

void
RecursivelySuspensibleThread::Start(bool suspended)
{
  suspend_count = suspended ? 1 : 0;
  SuspensibleThread::Start(suspended);
}

void
RecursivelySuspensibleThread::BeginSuspend() noexcept
{
  ++suspend_count;
  if (suspend_count == 1)
    SuspensibleThread::BeginSuspend();
}

void
RecursivelySuspensibleThread::Suspend() noexcept
{
  ++suspend_count;

  /* we don't check suspend_count==1 here, because the previous caller
     may have called BeginSuspend(), and this method is synchronous */
  SuspensibleThread::Suspend();
}

void
RecursivelySuspensibleThread::Resume() noexcept
{
  assert(suspend_count > 0);

  --suspend_count;
  if (suspend_count == 0)
    /* resume when all calls to BeginSuspend() / Suspend() have been
       undone */
    SuspensibleThread::Resume();
}
