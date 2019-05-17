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

#include "Thread/WorkerThread.hpp"
#include "Time/PeriodClock.hpp"

WorkerThread::WorkerThread(const char *_name,
                           unsigned _period_min, unsigned _idle_min,
                           unsigned _delay)
  :SuspensibleThread(_name),
   period_min(std::chrono::milliseconds(_period_min)),
   idle_min(std::chrono::milliseconds(_idle_min)),
   delay(std::chrono::milliseconds(_delay))
{
}

void
WorkerThread::Run() noexcept
{
  PeriodClock clock;

  std::unique_lock<Mutex> lock(mutex);

  while (true) {
    /* wait for work */
    if (!trigger_flag) {
      if (_CheckStoppedOrSuspended(lock))
        break;

      /* check trigger_flag again to avoid a race condition, because
         _CheckStoppedOrSuspended() may have unlocked the mutex while
         we were suspended */
      if (!trigger_flag)
        trigger_cond.wait(lock);
    }

    /* got the "stop" trigger? */
    if (delay.count() > 0
        ? _WaitForStopped(lock, delay)
        : _CheckStoppedOrSuspended(lock))
      break;

    if (!trigger_flag)
      continue;

    trigger_flag = false;

    {
      const ScopeUnlock unlock(mutex);

      /* do the actual work */
      if (period_min.count() > 0)
        clock.Update();

      Tick();
    }

    auto idle = idle_min;
    if (period_min.count() > 0) {
      const auto elapsed = clock.Elapsed();
      if (elapsed + idle < period_min)
        idle = period_min - elapsed;
    }

    if (idle.count() > 0 && _WaitForStopped(lock, idle))
      break;
  }
}
