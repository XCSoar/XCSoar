/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_EVENT_WINDOWS_KEY_CODE_HPP
#define XCSOAR_EVENT_WINDOWS_KEY_CODE_HPP

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
  KEY_KP0 = VK_NUMPAD0,  //Numlock is off by default for X11.   XK_KP_0,
  KEY_KP1 = VK_NUMPAD1,
  KEY_KP2 = VK_NUMPAD2,
  KEY_KP3 = VK_NUMPAD3,
  KEY_KP4 = VK_NUMPAD4,
  KEY_KP5 = VK_NUMPAD5,
  KEY_KP6 = VK_NUMPAD6,
  KEY_KP7 = VK_NUMPAD7,
  KEY_KP8 = VK_NUMPAD8,
  KEY_KP9 = VK_NUMPAD9,
  KEY_KPEQUAL = 0x30, //'0' key: windows sends shift and '0' keydown ignore shift
  KEY_KPENTER = VK_RETURN,
  KEY_KPASTERISK = VK_MULTIPLY,
  KEY_KPMINUS = VK_SUBTRACT,
  KEY_KPPLUS = VK_OEM_PLUS,
  KEY_KPSLASH =   VK_DIVIDE,
  KEY_BEGIN = VK_CLEAR,
  KEY_NUMLOCK = VK_NUMLOCK,
  KEY_PAGEUP = VK_PRIOR,
  KEY_PAGEDOWN = VK_NEXT,
  KEY_INSERT = VK_INSERT,
  KEY_DELETE = VK_DELETE,
  KEY_KPCOMMA = VK_DECIMAL,
};

#endif
