// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/display/Display.hpp"
#include "ui/event/Queue.hpp"

class ScreenGlobalInit {
  UI::Display display;

#ifdef USE_POLL_EVENT
  UI::EventQueue event_queue{display};
#else
  UI::EventQueue event_queue;
#endif

public:
  ScreenGlobalInit();
  ~ScreenGlobalInit();

  auto &GetDisplay() noexcept {
    return display;
  }

  auto &GetEventQueue() noexcept {
    return event_queue;
  }
};
