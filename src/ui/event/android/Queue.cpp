/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Queue.hpp"

namespace UI {

void
EventQueue::Inject(const Event &event) noexcept
{
  std::lock_guard<Mutex> lock(mutex);
  if (quit)
    return;

  events.push(event);
  cond.notify_one();
}

bool
EventQueue::Pop(Event &event) noexcept
{
  std::lock_guard<Mutex> lock(mutex);
  if (quit || events.empty())
    return false;

  event = events.front();
  events.pop();
  return true;
}

bool
EventQueue::Generate(Event &event) noexcept
{
  Timer *timer = timers.Pop(SteadyNow());
  if (timer != nullptr) {
    event.type = Event::TIMER;
    event.ptr = timer;
    return true;
  }

  return false;
}

bool
EventQueue::Wait(Event &event) noexcept
{
  std::unique_lock<Mutex> lock(mutex);
  if (quit)
    return false;

  if (events.empty())
    FlushClockCaches();

  while (events.empty()) {
    if (Generate(event))
      return true;

    const auto timeout = timers.GetTimeout(SteadyNow());
    if (timeout < std::chrono::steady_clock::duration::zero())
      cond.wait(lock);
    else
      cond.wait_for(lock, timeout);

    FlushClockCaches();
  }

  event = events.front();
  events.pop();
  return true;
}

void
EventQueue::Purge(bool (*match)(const Event &event, void *ctx) noexcept,
                  void *ctx) noexcept
{
  std::lock_guard<Mutex> lock(mutex);
  size_t n = events.size();
  while (n-- > 0) {
    if (!match(events.front(), ctx))
      events.push(events.front());
    events.pop();
  }
}

static bool
match_type(const Event &event, void *ctx) noexcept
{
  const Event::Type *type_p = (const Event::Type *)ctx;
  return event.type == *type_p;
}

void
EventQueue::Purge(Event::Type type) noexcept
{
  Purge(match_type, &type);
}

static bool
MatchCallback(const Event &event, void *ctx) noexcept
{
  const Event *match = (const Event *)ctx;
  return event.type == Event::CALLBACK && event.callback == match->callback &&
    event.ptr == match->ptr;
}

void
EventQueue::Purge(Event::Callback callback, void *ctx) noexcept
{
  Event match(callback, ctx);
  Purge(MatchCallback, (void *)&match);
}

void
EventQueue::AddTimer(Timer &timer, std::chrono::steady_clock::duration d) noexcept
{
  std::lock_guard<Mutex> lock(mutex);

  timers.Add(timer, SteadyNow() + d);

  cond.notify_one();
}

void
EventQueue::CancelTimer(Timer &timer) noexcept
{
  std::lock_guard<Mutex> lock(mutex);

  timers.Cancel(timer);
}

} // namespace UI
