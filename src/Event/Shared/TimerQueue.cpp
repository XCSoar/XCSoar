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

#include "TimerQueue.hpp"

int64_t
TimerQueue::GetTimeoutUS(uint64_t now_us) const
{
  auto i = timers.begin();
  if (i != timers.end()) {
    int64_t relative = i->due_us - now_us;
    if (relative <= 0)
      return 0;

    return relative;
  }

  return -1;
}

Timer *
TimerQueue::Pop(uint64_t now_us)
{
  auto t = timers.begin();
  if (t != timers.end() && t->IsDue(now_us)) {
    Timer *timer = t->timer;
    timers.erase(t);
    return timer;
  } else
    return nullptr;
}

void
TimerQueue::Add(Timer &timer, uint64_t due_us)
{
  timers.insert(TimerRecord(timer, due_us));
}

void
TimerQueue::Cancel(Timer &timer)
{
  for (auto i = timers.begin(), end = timers.end(); i != end; ++i) {
    if (i->timer == &timer) {
      timers.erase(i);
      return;
    }
  }
}
