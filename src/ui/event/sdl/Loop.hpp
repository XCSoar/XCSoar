// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace UI {

struct Event;
class EventQueue;
class TopWindow;

class EventLoop {
  EventQueue &queue;
  TopWindow *top_window;

  /**
   * True if working on a bulk of events.  At the end of that bulk,
   * TopWindow::validate() gets called.
   */
  bool bulk;

public:
  using Callback = void (*)(void *ctx) noexcept;

  EventLoop(EventQueue &_queue, TopWindow &_top_window)
    :queue(_queue), top_window(&_top_window), bulk(true) {}

  explicit EventLoop(EventQueue &_queue)
    :queue(_queue), top_window(nullptr), bulk(true) {}

  EventLoop(const EventLoop &) = delete;

  bool Get(Event &event);
  void Dispatch(const Event &event);
};

} // namespace UI
