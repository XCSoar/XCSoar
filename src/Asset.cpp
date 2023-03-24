// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Asset.hpp"

#ifdef USE_CONSOLE
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

#if defined(USE_CONSOLE) && !defined(KOBO)

bool
HasPointer() noexcept
{
  return UI::event_queue->HasPointer();
}

#endif

#ifdef USE_LIBINPUT

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
