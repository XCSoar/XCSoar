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

#include "Thread/StandbyThread.hpp"

StandbyThread::StandbyThread()
  :alive(false), pending(false), busy(false), stop(false) {}

StandbyThread::~StandbyThread()
{
  assert(!alive);
  assert(!busy);
}

void
StandbyThread::Trigger()
{
  assert(!IsInside());
  assert(!IsBusy());

  stop = false;
  pending = true;

  if (alive)
    TriggerCommand();
  else
    /* start it if it's not running currently */
    alive = Start();
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
StandbyThread::WaitDone()
{
  assert(!IsInside());

  while (alive && IsBusy()) {
#ifdef HAVE_POSIX
    cond.Wait(mutex);
#else
    done_trigger.Reset();
    mutex.Unlock();
    done_trigger.Wait();
    mutex.Lock();
#endif
  }
}

void
StandbyThread::WaitStopped()
{
  assert(!IsInside());
  assert(stop);

  /* mutex must be unlocked because Thread::Join() blocks */
  mutex.Unlock();
  Thread::Join();
  mutex.Lock();
}

void
StandbyThread::Run()
{
  assert(!busy);

  mutex.Lock();
  alive = true;

  while (!stop) {
    assert(!busy);

    if (!pending) {
      /* wait for a command */
#ifdef HAVE_POSIX
      cond.Wait(mutex);
#else
      command_trigger.Reset();
      mutex.Unlock();
      command_trigger.Wait();
      mutex.Lock();
#endif
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
  mutex.Unlock();
}

