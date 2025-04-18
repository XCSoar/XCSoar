// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TimerQueue.hpp"

namespace UI {

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

} // namespace UI
