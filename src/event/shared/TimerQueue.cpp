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

std::chrono::steady_clock::duration
TimerQueue::GetTimeout(std::chrono::steady_clock::time_point now) const noexcept
{
  auto i = timers.begin();
  if (i != timers.end()) {
    auto relative = i->due - now;
    if (relative <= std::chrono::steady_clock::duration::zero())
      return {};

    return relative;
  }

  return std::chrono::steady_clock::duration(-1);
}

Timer *
TimerQueue::Pop(std::chrono::steady_clock::time_point now) noexcept
{
  auto t = timers.begin();
  if (t != timers.end() && t->IsDue(now)) {
    Timer *timer = t->timer;
    timers.erase(t);
    return timer;
  } else
    return nullptr;
}

void
TimerQueue::Add(Timer &timer, std::chrono::steady_clock::time_point due) noexcept
{
  timers.insert(TimerRecord(timer, due));
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
