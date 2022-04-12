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

#ifndef XCSOAR_EVENT_X11_KEY_CODE_HPP
#define XCSOAR_EVENT_X11_KEY_CODE_HPP

#include <X11/keysym.h>

enum {
  KEY_RETURN = XK_Return,
  KEY_ESCAPE = XK_Escape,
  KEY_BACKSPACE = XK_BackSpace,
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
  KEY_NUMLOCK = XK_Num_Lock,
  KEY_KPENTER = XK_KP_Enter,
  KEY_KPASTERISK = XK_KP_Multiply,
  KEY_KPPLUS = XK_KP_Add,
  KEY_KPCOMMA = XK_KP_Separator,
  KEY_KPMINUS = XK_KP_Subtract,
  KEY_KPSLASH = XK_KP_Divide,
  KEY_KP0 = XK_KP_0,  //Numlock is off by default for X11.   XK_KP_0,
  KEY_KP1 = XK_KP_1,
  KEY_KP2 = XK_KP_2,
  KEY_KP3 = XK_KP_3,
  KEY_KP4 = XK_KP_4,
  KEY_KP5 = XK_KP_5,
  KEY_KP6 = XK_KP_6,
  KEY_KP7 = XK_KP_7,
  KEY_KP8 = XK_KP_8,
  KEY_KP9 = XK_KP_9,
  KEY_KPEQUAL= XK_KP_Equal,
  KEY_KPDEL=XK_KP_9,
  KEY_KP_INSERT = XK_KP_Insert,  //Numlock is off by default for X11.   XK_KP_0,
  KEY_KP_END = XK_KP_End,
  KEY_KP_DOWN = XK_KP_Down,
  KEY_KP_PAGE_DOWN = XK_KP_Page_Down,
  KEY_KP_LEFT = XK_KP_Left,
  KEY_KP_BEGIN = XK_KP_Begin,
  KEY_KP_RIGHT = XK_KP_Right,
  KEY_KP_HOME = XK_KP_Home,
  KEY_KP_UP = XK_KP_Up,
  KEY_KP_PAGE_UP = XK_KP_Page_Up,
  KEY_APP1,
  KEY_APP2,
  KEY_APP3,
  KEY_APP4,
  KEY_APP5,
  KEY_APP6,
};

#endif
