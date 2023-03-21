// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "thread/StandbyThread.hpp"

StandbyThread::StandbyThread(const char *_name)
  :Thread(_name) {}

StandbyThread::~StandbyThread()
{
  assert(!alive);
  assert(!busy);
}

void
StandbyThread::Trigger()
{
  assert(!IsInside());

  stop = false;
  pending = true;

  if (alive)
    TriggerCommand();
  else {
    /* start it if it's not running currently */
    Start();
    alive = true;
  }
}

void
StandbyThread::StopAsync()
{
  assert(!IsInside());

  stop = true;

  /* clear the queued work */
  pending = false;

  TriggerCommand();
}

void
StandbyThread::WaitDone(std::unique_lock<Mutex> &lock) noexcept
{
  assert(!IsInside());

  cond.wait(lock, [this]{ return !alive || !IsBusy(); });
}

void
StandbyThread::WaitStopped()
{
  assert(!IsInside());
  assert(stop);

  if (!IsDefined())
    /* was never started */
    return;

  /* mutex must be unlocked because Thread::Join() blocks */
  const ScopeUnlock unlock(mutex);
  Thread::Join();
}

void
StandbyThread::Run() noexcept
{
  assert(!busy);

  std::unique_lock lock{mutex};

  alive = true;

  while (!stop) {
    assert(!busy);

    if (!pending) {
      /* wait for a command */
      cond.wait(lock);
    }

    assert(!busy);

    if (pending) {
      /* there's work to do */
      pending = false;
      busy = true;
      Tick();
      busy = false;
      TriggerDone();
    }
  }

  alive = false;
  TriggerDone();
}

