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

bool
EventQueue::Wait(Event &event)
{
  assert(InMainThread());

  FlushClockCaches();

  while (true) {
    ::ResetEvent(trigger);

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
