/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include <windows.h>

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

#endif
