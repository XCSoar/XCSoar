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

#ifndef XCSOAR_EVENT_X11_KEY_CODE_HPP
#define XCSOAR_EVENT_X11_KEY_CODE_HPP

#include <X11/keysym.h>

enum {
  KEY_RETURN = XK_Return,
  KEY_ESCAPE = XK_Escape,
  KEY_PRIOR = XK_Page_Up,
  KEY_NEXT = XK_Page_Down,
  KEY_UP = XK_Up,
  KEY_DOWN = XK_Down,
  KEY_LEFT = XK_Left,
  KEY_RIGHT = XK_Right,
  KEY_HOME = XK_Home,
  KEY_END = XK_End,
  KEY_TAB = XK_Tab,
  KEY_BACK = XK_BackSpace,
  KEY_SPACE = XK_space,
  KEY_F1 = XK_F1,
  KEY_F2 = XK_F2,
  KEY_F3 = XK_F3,
  KEY_F4 = XK_F4,
  KEY_F5 = XK_F5,
  KEY_F6 = XK_F6,
  KEY_F7 = XK_F7,
  KEY_F8 = XK_F8,
  KEY_F9 = XK_F9,
  KEY_F10 = XK_F10,
  KEY_F11 = XK_F11,
  KEY_F12 = XK_F12,
  KEY_MENU = XK_Menu,
  KEY_APP1,
  KEY_APP2,
  KEY_APP3,
  KEY_APP4,
  KEY_APP5,
  KEY_APP6,
};

#endif
