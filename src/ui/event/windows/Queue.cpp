// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Queue.hpp"
#include "Event.hpp"
#include "../Timer.hpp"
#include "thread/Debug.hpp"

#include <winbase.h> // for INFINITE

namespace UI {

EventQueue::EventQueue()
  :trigger(::CreateEvent(nullptr, true, false, nullptr)) {}

void
EventQueue::InjectCall(Event::Callback callback, void *ctx) noexcept
{
  {
    const std::lock_guard lock{mutex};
    injected_events.push({callback, ctx});
  }

  WakeUp();
}

bool
EventQueue::PopInjected(Event &event) noexcept
{
  const std::lock_guard lock{mutex};
  if (injected_events.empty())
    return false;

  const InjectedEvent &injected = injected_events.front();
  event.callback = injected.callback;
  event.callback_ctx = injected.ctx;
  injected_events.pop();
  return true;
}

void
EventQueue::Purge(Event::Callback callback, void *ctx) noexcept
{
  const std::lock_guard lock{mutex};
  size_t n = injected_events.size();
  while (n-- > 0) {
    const InjectedEvent &injected = injected_events.front();
    if (injected.callback != callback || injected.ctx != ctx)
      injected_events.push(injected);
    injected_events.pop();
  }
}

bool
EventQueue::Wait(Event &event)
{
  assert(InMainThread());

  FlushClockCaches();

  while (true) {
    ::ResetEvent(trigger);

    event.callback = nullptr;
    event.callback_ctx = nullptr;

    if (PopInjected(event))
      return true;

    /* invoke all due timers */

    const auto now = SteadyNow();
    std::chrono::steady_clock::duration timeout;
    while (true) {
      timeout = timers.GetTimeout(now);
      if (timeout != std::chrono::steady_clock::duration::zero())
        break;

      Timer *timer = timers.Pop(now);
      if (timer == nullptr)
        break;

      timer->Invoke();
    }

    /* check for WIN32 event */

    if (::PeekMessage(&event.msg, nullptr, 0, 0, PM_REMOVE))
      return event.msg.message != WM_QUIT;

    const DWORD n = 1;
    const LPHANDLE handles = &trigger;

    const DWORD timeout_ms = timeout >= std::chrono::steady_clock::duration::zero()
      ? DWORD(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()) + 1
      : INFINITE;

    DWORD result = ::MsgWaitForMultipleObjects(n, handles, false,
                                               timeout_ms, QS_ALLEVENTS);
    if (result == 0xffffffff)
      return false;

    FlushClockCaches();
  }
}

static void
HandleMessages(UINT wMsgFilterMin, UINT wMsgFilterMax)
{
  MSG msg;
  while (::PeekMessage(&msg, NULL, wMsgFilterMin, wMsgFilterMax, PM_REMOVE)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
}

void
EventQueue::HandlePaintMessages()
{
  HandleMessages(WM_SIZE, WM_SIZE);
  HandleMessages(WM_PAINT, WM_PAINT);
}

void
EventQueue::AddTimer(Timer &timer, std::chrono::steady_clock::duration d) noexcept
{
  const std::lock_guard lock{mutex};

  const auto due = SteadyNow() + d;
  timers.Add(timer, due);

  if (timers.IsBefore(due))
    WakeUp();
}

void
EventQueue::CancelTimer(Timer &timer)
{
  const std::lock_guard lock{mutex};

  timers.Cancel(timer);
}

} // namespace UI
