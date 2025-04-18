// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class AsioThread;
class EventThread;
namespace Cares { class Channel; }

extern AsioThread *asio_thread;
extern Cares::Channel *global_cares_channel;

void
InitialiseAsioThread();

void
DeinitialiseAsioThread();

class ScopeGlobalAsioThread {
public:
  ScopeGlobalAsioThread() {
    InitialiseAsioThread();
  }

  ~ScopeGlobalAsioThread() {
    DeinitialiseAsioThread();
  }
};
