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

/* these are defined in linux/input.h but we redefine them in our enum */
#undef KEY_LEFT
#undef KEY_RIGHT

/* function keys and other keys from linux/input.h that we redefine */
#undef KEY_F1
#undef KEY_F2
#undef KEY_F3
#undef KEY_F4
#undef KEY_F5
#undef KEY_F6
#undef KEY_F7
#undef KEY_F8
#undef KEY_F9
#undef KEY_F10
#undef KEY_F11
#undef KEY_F12
#undef KEY_TAB
#undef KEY_MENU
#endif

enum {
#if defined(KOBO) || defined(USE_LIBINPUT) || defined(USE_WAYLAND)
  KEY_RETURN = KEY_ENTER,
  KEY_ESCAPE = KEY_ESC,
  KEY_PRIOR = KEY_PAGEUP,
  KEY_NEXT = KEY_PAGEDOWN,
  KEY_BACK = KEY_BACKSPACE,
  KEY_F1 = 59,    /* Linux KEY_F1 */
  KEY_F2 = 60,    /* Linux KEY_F2 */
  KEY_F3 = 61,    /* Linux KEY_F3 */
  KEY_F4 = 62,    /* Linux KEY_F4 */
  KEY_F5 = 63,    /* Linux KEY_F5 */
  KEY_F6 = 64,    /* Linux KEY_F6 */
  KEY_F7 = 65,    /* Linux KEY_F7 */
  KEY_F8 = 66,    /* Linux KEY_F8 */
  KEY_F9 = 67,    /* Linux KEY_F9 */
  KEY_F10 = 68,   /* Linux KEY_F10 */
  KEY_F11 = 87,   /* Linux KEY_F11 */
  KEY_F12 = 88,   /* Linux KEY_F12 */
  KEY_TAB = 15,   /* Linux KEY_TAB */
  KEY_MENU = 139, /* Linux KEY_MENU - context menu key */
  /* Note: Linux input event codes KEY_UP=103 and KEY_DOWN=108 conflict
     with ASCII 'g' and 'l'. We use the raw Linux codes here, but
     WaylandQueue must check raw key codes before translation */
  KEY_UP = 103,    /* Linux KEY_UP - conflicts with 'g' (103) */
  KEY_DOWN = 108,  /* Linux KEY_DOWN - conflicts with 'l' (108) */
  KEY_LEFT = 105,  /* Linux KEY_LEFT - conflicts with 'i' (105) */
  KEY_RIGHT = 106, /* Linux KEY_RIGHT - conflicts with 'j' (106) */
  /* KEY_APP1-6 need explicit values above 200 to avoid conflicts with
     ASCII and Linux key codes */
  KEY_APP1 = 201,
  KEY_APP2 = 202,
  KEY_APP3 = 203,
  KEY_APP4 = 204,
  KEY_APP5 = 205,
  KEY_APP6 = 206,
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
  KEY_APP1,
  KEY_APP2,
  KEY_APP3,
  KEY_APP4,
  KEY_APP5,
  KEY_APP6,
#endif
};
