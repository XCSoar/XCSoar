// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <windef.h> // for HWND

namespace UI {

struct Event;
class EventQueue;

class EventLoop {
  EventQueue &queue;

public:
  explicit EventLoop(EventQueue &_queue):queue(_queue) {}

  EventLoop(const EventLoop &) = delete;

  bool Get(Event &msg);
  void Dispatch(const Event &msg);
};

class DialogEventLoop : public EventLoop {
  HWND dialog;

public:
  DialogEventLoop(EventQueue &_loop, HWND _dialog)
    :EventLoop(_loop), dialog(_dialog) {}

  void Dispatch(Event &msg);
};

} // namespace UI
