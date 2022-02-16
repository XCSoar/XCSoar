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
#include "DisplayOrientation.hpp"
#include "event/SignalMonitor.hxx"

#ifdef MESA_KMS
#include "ui/display/Display.hpp"
#endif

namespace UI {

EventQueue::EventQueue(Display &_display) noexcept
#if defined(USE_X11) || defined(USE_WAYLAND) || defined(MESA_KMS)
  :display(_display)
#endif
{
  SignalMonitorInit(event_loop);
  SignalMonitorRegister(SIGINT, BIND_THIS_METHOD(OnQuitSignal));
  SignalMonitorRegister(SIGTERM, BIND_THIS_METHOD(OnQuitSignal));
  SignalMonitorRegister(SIGQUIT, BIND_THIS_METHOD(OnQuitSignal));
}

EventQueue::~EventQueue() noexcept
{
  SignalMonitorFinish();
}

void
EventQueue::Push(const Event &event) noexcept
{
  event_loop.Finish();

  std::lock_guard<Mutex> lock(mutex);
  events.push(event);
}

void
EventQueue::Interrupt() noexcept
{
  {
    std::lock_guard<Mutex> lock(mutex);
    if (!events.empty())
      return;

    events.push(Event::NOP);
  }

  event_loop.Finish();
}

void
EventQueue::Inject(const Event &event) noexcept
{
  std::lock_guard<Mutex> lock(mutex);
  events.push(event);
  WakeUp();
}

void
EventQueue::Poll() noexcept
{
  event_loop.ResetFinish();
  event_loop.Run();
}

bool
EventQueue::Generate(Event &event) noexcept
{
#ifndef NON_INTERACTIVE
  if (input_queue.Generate(event))
    return true;
#endif

#ifdef MESA_KMS
  if (display.CheckDirty()) {
    event = Event::EXPOSE;
    return true;
  }
#endif

  return false;
}

bool
EventQueue::Pop(Event &event) noexcept
{
  if (quit)
    return false;

  std::lock_guard<Mutex> lock(mutex);
  if (events.empty()) {
    return Generate(event);
  }

  event = events.front();
  events.pop();

  return true;
}

bool
EventQueue::Wait(Event &event) noexcept
{
  if (quit)
    return false;

  std::lock_guard<Mutex> lock(mutex);

  if (events.empty()) {
    if (Generate(event))
      return true;

    while (events.empty()) {
      {
        const ScopeUnlock unlock(mutex);
        Poll();
      }

      if (quit)
        return false;

      if (Generate(event))
        return true;
    }
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

} // namespace UI
