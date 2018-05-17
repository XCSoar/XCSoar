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

#include "RateLimiter.hpp"

#include <assert.h>

RateLimiter::RateLimiter(unsigned _period_ms, unsigned _delay_ms)
  :period_ms(_period_ms - _delay_ms), delay_ms(_delay_ms)
{
  assert(_period_ms >= _delay_ms);
}

void
RateLimiter::Trigger()
{
  if (IsActive())
    return;

  unsigned schedule_ms = delay_ms;
  int elapsed = clock.Elapsed();
  if (elapsed >= 0 && (unsigned)elapsed < period_ms)
    schedule_ms += period_ms - elapsed;

  Schedule(schedule_ms);
}

void
RateLimiter::OnTimer()
{
  Timer::Cancel();

  clock.Update();
  Run();
}
