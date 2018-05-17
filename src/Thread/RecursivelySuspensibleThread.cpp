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

#include "RecursivelySuspensibleThread.hpp"

#include <assert.h>

bool
RecursivelySuspensibleThread::Start(bool suspended)
{
  suspend_count = suspended ? 1 : 0;
  return SuspensibleThread::Start(suspended);
}

void
RecursivelySuspensibleThread::BeginSuspend()
{
  ++suspend_count;
  if (suspend_count == 1)
    SuspensibleThread::BeginSuspend();
}

void
RecursivelySuspensibleThread::Suspend()
{
  ++suspend_count;

  /* we don't check suspend_count==1 here, because the previous caller
     may have called BeginSuspend(), and this method is synchronous */
  SuspensibleThread::Suspend();
}

void
RecursivelySuspensibleThread::Resume()
{
  assert(suspend_count > 0);

  --suspend_count;
  if (suspend_count == 0)
    /* resume when all calls to BeginSuspend() / Suspend() have been
       undone */
    SuspensibleThread::Resume();
}
