// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Queue.hpp"
#include "Event.hpp"
#include "../Timer.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

namespace UI {

EventQueue::EventQueue() noexcept
  :quit(false) {}

void
EventQueue::Quit() noexcept
{
  quit.store(true);
  
#if defined(__APPLE__) && TARGET_OS_IPHONE
  /* On iOS, SDL_WaitEvent() may not return when the app is being terminated.
   * Push a dummy user event to wake up the event loop so it can detect the
   * quit flag and exit cleanly. */
  SDL_Event event;
  event.type = SDL_USEREVENT;
  event.user.code = 0;
  event.user.data1 = nullptr;
  event.user.data2 = nullptr;
  ::SDL_PushEvent(&event);
#endif
}

void
EventQueue::InjectCall(EventLoop::Callback callback, void *ctx) noexcept
{
  SDL_Event event;
  event.type = EVENT_CALLBACK;
  event.user.data1 = (void *)callback;
  event.user.data2 = ctx;
  ::SDL_PushEvent(&event);
}

static void
InvokeTimer(void *ctx) noexcept
{
  Timer *timer = (Timer *)ctx;
  timer->Invoke();
}

bool
EventQueue::Generate(Event &event) noexcept
{
  Timer *timer = timers.Pop(SteadyNow());
  if (timer != nullptr) {
    event.event.type = EVENT_CALLBACK;
    event.event.user.data1 = (void *)InvokeTimer;
    event.event.user.data2 = (void *)timer;
    return true;
  }

  return false;
}

bool
EventQueue::Pop(Event &event) noexcept
{
  return !quit.load() && (Generate(event) || ::SDL_PollEvent(&event.event));
}

bool
EventQueue::Wait(Event &event) noexcept
{
  /* this busy loop is ugly, and I wish we could do better than that,
     but SDL_WaitEvent() is just as bad; however copying this busy
     loop allows us to plug in more event sources */

  if (quit.load())
    return false;

  while (true) {
    if (Generate(event))
      return true;

    ::SDL_PumpEvents();

    // Check quit again after pumping events, as it may have been set
    if (quit.load())
      return false;

    const auto timeout = timers.GetTimeout(SteadyNow());

    int result = timeout.count() >= 0
      ? SDL_WaitEventTimeout(&event.event,
                             std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count())
      : SDL_WaitEvent(&event.event);
    if (result != 0)
      return result > 0;

    FlushClockCaches();
  }
}

void
EventQueue::Purge(Uint32 event,
                  bool (*match)(const SDL_Event &event, void *ctx) noexcept,
                  void *ctx) noexcept
{
  SDL_Event events[256]; // is that enough?
  int count = SDL_PeepEvents(events, 256, SDL_GETEVENT, event, event);
  assert(count >= 0);

  SDL_Event *dest = events;
  for (const SDL_Event *src = events, *end = src + count; src != end; ++src)
    if (!match(*src, ctx))
      *dest++ = *src;

  SDL_PeepEvents(events, dest - events, SDL_ADDEVENT, event, event);
}

struct MatchCallbackData {
  void *data1, *data2;
};

static bool
MatchCallback(const SDL_Event &event, void *ctx) noexcept
{
  const MatchCallbackData *data = (const MatchCallbackData *)ctx;
  return event.type == EVENT_CALLBACK && event.user.data1 == data->data1 &&
    event.user.data2 == data->data2;
}

void
EventQueue::Purge(EventLoop::Callback callback, void *ctx) noexcept
{
  MatchCallbackData data { (void *)callback, ctx };
  Purge(EVENT_CALLBACK, MatchCallback, (void *)&data);
}

void
EventQueue::AddTimer(Timer &timer, std::chrono::steady_clock::duration d) noexcept
{
  const std::lock_guard lock{mutex};

  timers.Add(timer, SteadyNow() + d);
}

void
EventQueue::CancelTimer(Timer &timer) noexcept
{
  const std::lock_guard lock{mutex};

  timers.Cancel(timer);
}

} // namespace UI
