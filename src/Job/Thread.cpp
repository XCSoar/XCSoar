// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Thread.hpp"
#include "Job.hpp"

void
JobThread::Start()
{
  Thread::Start();
  was_running = true;
}

void
JobThread::Run() noexcept
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
