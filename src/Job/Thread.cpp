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

#include "Thread.hpp"
#include "Job.hpp"

bool
JobThread::Start()
{
  if (!Thread::Start())
    return false;

  was_running = true;
  return true;
}

void
JobThread::Run()
{
  assert(!running.load(std::memory_order_relaxed));

  running.store(true, std::memory_order_relaxed);

  try {
    job.Run(*this);
  } catch (...) {
    /* an exception was thrown by the Job: remember it, rethrow it in
       the calling thread in Join() */
    exception = std::current_exception();
  }

  running.store(false, std::memory_order_relaxed);

  SendNotification();
}

void
JobThread::OnNotification()
{
  ThreadedOperationEnvironment::OnNotification();

  if (was_running && !running.load(std::memory_order_relaxed)) {
    OnComplete();
    was_running = false;
  }
}

void
JobThread::Join()
{
  Thread::Join();

  if (exception)
    /* rethrow the exception that was thrown by Job::Run() in the
       calling thread */
    std::rethrow_exception(std::move(exception));
}
