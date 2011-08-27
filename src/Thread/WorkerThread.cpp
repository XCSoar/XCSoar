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

#include "Thread/WorkerThread.hpp"
#include "Thread/Trigger.hpp"
#include "PeriodClock.hpp"

WorkerThread::WorkerThread(unsigned _period_min, unsigned _idle_min,
                           unsigned _delay)
  :period_min(_period_min), idle_min(_idle_min), delay(_delay) {
}

void
WorkerThread::Run()
{
  PeriodClock clock;

  while (true) {
    /* wait for work */
    event_trigger.Wait();

    /* got the "stop" trigger? */
    if (delay > 0
        ? WaitForStopped(delay)
        : CheckStoppedOrSuspended())
      break;

    /* reset the trigger here, because our client might have called
       Trigger() a few more times while we were suspended in
       CheckStoppedOrSuspended() */
    event_trigger.Reset();

    if (IsCommandPending()) {
      /* just in case we got another suspend/stop command after
         CheckStoppedOrSuspended() returned and before the trigger got
         reset: restore the trigger and skip this iteration, to fix
         the race condition */
      event_trigger.Signal();
      continue;
    }

    /* do the actual work */
    if (period_min > 0)
      clock.update();

    Tick();

    unsigned idle = idle_min;
    if (period_min > 0) {
      unsigned elapsed = clock.elapsed();
      if (elapsed + idle < period_min)
        idle = period_min - elapsed;
    }

    if (idle > 0 && WaitForStopped(idle))
      break;
  }
}
