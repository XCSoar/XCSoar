// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyboardDetection.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#ifdef ENABLE_SDL
#include "ui/event/sdl/KeyCode.hpp"
#endif

bool has_cursor_keys = false;

void
DetectCursorKeysiOS(unsigned key_code)
{
#ifdef ENABLE_SDL
  // Check if this is a cursor key (UP, DOWN, LEFT, RIGHT)
  if (!has_cursor_keys && 
      (key_code == KEY_UP ||
       key_code == KEY_DOWN ||
       key_code == KEY_LEFT ||
       key_code == KEY_RIGHT)) {
      has_cursor_keys = true;
  }
#endif
}

#endif // TARGET_OS_IPHONE
#endif // __APPLE__
