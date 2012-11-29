/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Async.hpp"
#include "Job.hpp"
#include "Operation/ThreadedOperationEnvironment.hpp"
#include "Event/Notify.hpp"

void
AsyncJobRunner::Start(Job *_job, OperationEnvironment &_env, Notify *_notify)
{
  assert(_job != NULL);
  assert(!IsBusy());

  job = _job;
  env = new ThreadedOperationEnvironment(_env);
  notify = _notify;

  running.store(true, std::memory_order_relaxed);
  Thread::Start();
}

void
AsyncJobRunner::Cancel()
{
  assert(IsBusy());

  env->Cancel();

  if (notify != NULL)
    /* make sure the notification doesn't get delivered, even if
       this method was invoked too late */
    notify->ClearNotification();
}

#if defined(__clang__) || GCC_VERSION >= 40700
/* no, ThreadedOperationEnvironment really doesn't need a virtual
   destructor */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif

Job *
AsyncJobRunner::Wait()
{
  assert(IsBusy());

  Thread::Join();

  delete env;

  return job;
}

#if defined(__clang__) || GCC_VERSION >= 40700
#pragma GCC diagnostic pop
#endif

void
AsyncJobRunner::Run()
{
  assert(IsInside());
  assert(running.load(std::memory_order_relaxed));

  job->Run(*env);

  if (notify != NULL && !env->IsCancelled())
    notify->SendNotification();

  running.store(false, std::memory_order_relaxed);
}
