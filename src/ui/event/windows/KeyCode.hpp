// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <windef.h> // for HWND (needed by winuser.h)
#include <winuser.h>

enum {
  KEY_SPACE = VK_SPACE,
  KEY_UP = VK_UP,
  KEY_DOWN = VK_DOWN,
  KEY_LEFT = VK_LEFT,
  KEY_RIGHT = VK_RIGHT,
  KEY_HOME = VK_HOME,
  KEY_END = VK_END,
  KEY_PRIOR = VK_PRIOR,
  KEY_NEXT = VK_NEXT,
  KEY_RETURN = VK_RETURN,
  KEY_F1 = VK_F1,
  KEY_F2 = VK_F2,
  KEY_F3 = VK_F3,
  KEY_F4 = VK_F4,
  KEY_F5 = VK_F5,
  KEY_F6 = VK_F6,
  KEY_F7 = VK_F7,
  KEY_F8 = VK_F8,
  KEY_F9 = VK_F9,
  KEY_F10 = VK_F10,
  KEY_F11 = VK_F11,
  KEY_F12 = VK_F12,
  /* These Keys are used for the Triadis-RemoteStick, as well as for
     expanded Keyboard-Events */
  KEY_F13 = VK_F13,
  KEY_F14 = VK_F14,
  KEY_F15 = VK_F15,
  KEY_F16 = VK_F16,
  KEY_F17 = VK_F17,
  KEY_F18 = VK_F18,
  KEY_F19 = VK_F19,
  KEY_F20 = VK_F20,
  KEY_ESCAPE = VK_ESCAPE,
  KEY_TAB = VK_TAB,
  KEY_BACK = VK_BACK,
  KEY_MENU = VK_MENU,
  KEY_APP1 = '1',
  KEY_APP2 = '2',
  KEY_APP3 = '3',
  KEY_APP4 = '4',
  KEY_APP5 = '5',
  KEY_APP6 = '6',
};
