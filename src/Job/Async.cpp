// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Async.hpp"
#include "Job.hpp"
#include "Operation/ThreadedOperationEnvironment.hpp"
#include "ui/event/Notify.hpp"

void
AsyncJobRunner::Start(Job *_job, OperationEnvironment &_env,
                      UI::Notify *_notify)
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

#ifdef __GNUC__
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

  if (exception)
    /* rethrow the exception that was thrown by Job::Run() in the
       calling thread */
    std::rethrow_exception(std::move(exception));

  return job;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

void
AsyncJobRunner::Run() noexcept
{
  assert(IsInside());
  assert(running.load(std::memory_order_relaxed));

  try {
    job->Run(*env);
  } catch (...) {
    /* an exception was thrown by the Job: remember it, rethrow it in
       the calling thread in Wait() */
    exception = std::current_exception();
  }

  if (notify != NULL && !env->IsCancelled())
    notify->SendNotification();

  running.store(false, std::memory_order_relaxed);
}
