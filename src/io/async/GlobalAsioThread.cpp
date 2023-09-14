// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlobalAsioThread.hpp"
#include "AsioThread.hpp"
#include "event/net/cares/Channel.hxx"

AsioThread *asio_thread;
Cares::Channel *global_cares_channel;

void
InitialiseAsioThread()
{
  assert(asio_thread == nullptr);

  asio_thread = new AsioThread();
  global_cares_channel = new Cares::Channel(*asio_thread);
  asio_thread->Start();
}

void
DeinitialiseAsioThread()
{
  delete std::exchange(global_cares_channel, nullptr);

  asio_thread->Stop();
  delete asio_thread;
  asio_thread = nullptr;
}
