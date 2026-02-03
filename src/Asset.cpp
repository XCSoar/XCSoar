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

static bool touch_screen_override_valid = false;
static bool touch_screen_override_value = false;

void
SetTouchScreenOverride(bool value) noexcept
{
  touch_screen_override_valid = true;
  touch_screen_override_value = value;
}

bool
HasTouchScreen() noexcept
{
  if (touch_screen_override_valid)
    return touch_screen_override_value;
  return UI::event_queue->HasTouchScreen();
}

bool
HasKeyboard() noexcept
{
  return UI::event_queue->HasKeyboard();
}

#endif
