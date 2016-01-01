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

#include "Queue.hpp"
#include "Event.hpp"
#include "../Timer.hpp"
#include "Thread/Debug.hpp"
#include "OS/Clock.hpp"

EventQueue::EventQueue()
  :now_us(MonotonicClockUS()),
   trigger(::CreateEvent(nullptr, true, false, nullptr)) {}

bool
EventQueue::Wait(Event &event)
{
  assert(InMainThread());

  now_us = MonotonicClockUS();

  while (true) {
    ::ResetEvent(trigger);

    /* invoke all due timers */

    int64_t timeout_us;
    while (true) {
      timeout_us = timers.GetTimeoutUS(now_us);
      if (timeout_us != 0)
        break;

      Timer *timer = timers.Pop(now_us);
      if (timer == nullptr)
        break;

      timer->Invoke();
    }

    /* check for WIN32 event */

    if (::PeekMessage(&event.msg, nullptr, 0, 0, PM_REMOVE))
      return event.msg.message != WM_QUIT;

    const DWORD n = 1;
    const LPHANDLE handles = &trigger;

    const DWORD timeout = timeout_us >= 0
      ? DWORD(timeout_us / 1000) + 1
      : INFINITE;

    DWORD result = ::MsgWaitForMultipleObjects(n, handles, false,
                                               timeout, QS_ALLEVENTS);
    if (result == 0xffffffff)
      return false;

    now_us = MonotonicClockUS();
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
EventQueue::AddTimer(Timer &timer, unsigned ms)
{
  ScopeLock protect(mutex);

  const uint64_t due_us = MonotonicClockUS() + ms * 1000;
  timers.Add(timer, due_us);

  if (timers.IsBefore(due_us))
    WakeUp();
}

void
EventQueue::CancelTimer(Timer &timer)
{
  ScopeLock protect(mutex);

  timers.Cancel(timer);
}
