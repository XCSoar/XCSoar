// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AsioThread.hpp"

void
AsioThread::Start()
{
  assert(!IsDefined());

  event_loop.SetAlive(true);
  Thread::Start();
}

void
AsioThread::Stop()
{
  /* set the "stop" flag and wake up the thread */
  event_loop.InjectBreak();

  /* wait for the thread to finish */
  Join();
}

void
AsioThread::Run() noexcept
{
  event_loop.Run();
}
