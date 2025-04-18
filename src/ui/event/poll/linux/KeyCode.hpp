// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#if defined(KOBO) || defined(USE_LIBINPUT) || defined(USE_WAYLAND)
#include <linux/input.h>

/* these macros conflict with Event::Type */
#undef KEY_UP
#undef KEY_DOWN

/* wrong meaning */
#undef KEY_NEXT
#undef KEY_BACK
#endif

enum {
#if defined(KOBO) || defined(USE_LIBINPUT) || defined(USE_WAYLAND)
  KEY_RETURN = KEY_ENTER,
  KEY_ESCAPE = KEY_ESC,
  KEY_PRIOR = KEY_PAGEUP,
  KEY_NEXT = KEY_PAGEDOWN,
  KEY_BACK = KEY_BACKSPACE,
  KEY_UP = 103,
  KEY_DOWN = 108,
#else
  KEY_SPACE = ' ',
  KEY_UP = 0403,
  KEY_DOWN = 0402,
  KEY_LEFT = 0404,
  KEY_RIGHT = 0405,
  KEY_HOME = 0406,
  KEY_END = 0550,
  KEY_PRIOR = 0523,
  KEY_NEXT = 0522,
  KEY_RETURN = '\n',
  KEY_F1 = 0411,
  KEY_F2 = 0412,
  KEY_F3 = 0413,
  KEY_F4 = 0414,
  KEY_F5 = 0415,
  KEY_F6 = 0416,
  KEY_F7 = 0417,
  KEY_F8 = 0420,
  KEY_F9 = 0421,
  KEY_F10 = 0422,
  KEY_F11 = 0423,
  KEY_F12 = 0424,
  KEY_ESCAPE = 0x1b,
  KEY_TAB = '\t',
  KEY_BACK = 0407,
  KEY_MENU,
#endif
  KEY_APP1,
  KEY_APP2,
  KEY_APP3,
  KEY_APP4,
  KEY_APP5,
  KEY_APP6,
};
