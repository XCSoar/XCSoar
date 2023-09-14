// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Asset.hpp"

#if defined(USE_CONSOLE) || defined(USE_WAYLAND)
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

#if (defined(USE_CONSOLE) && !defined(KOBO)) || defined(USE_WAYLAND)

bool
HasPointer() noexcept
{
  return UI::event_queue->HasPointer();
}

#endif

#if defined(USE_LIBINPUT) || defined(USE_WAYLAND)

bool
HasTouchScreen() noexcept
{
  return UI::event_queue->HasTouchScreen();
}

bool
HasKeyboard() noexcept
{
  return UI::event_queue->HasKeyboard();
}

#endif
