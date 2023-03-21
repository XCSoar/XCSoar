// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Queue.hpp"

namespace UI {

void
EventQueue::Inject(const Event &event) noexcept
{
  const std::lock_guard lock{mutex};
  if (quit)
    return;

  events.push(event);
  cond.notify_one();
}

bool
EventQueue::Pop(Event &event) noexcept
{
  const std::lock_guard lock{mutex};
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
  std::unique_lock lock{mutex};
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
  const std::lock_guard lock{mutex};
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
  const std::lock_guard lock{mutex};

  timers.Add(timer, SteadyNow() + d);

  cond.notify_one();
}

void
EventQueue::CancelTimer(Timer &timer) noexcept
{
  const std::lock_guard lock{mutex};

  timers.Cancel(timer);
}

} // namespace UI
