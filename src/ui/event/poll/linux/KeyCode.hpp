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

#ifndef XCSOAR_EVENT_LINUX_KEY_CODE_HPP
#define XCSOAR_EVENT_LINUX_KEY_CODE_HPP

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
  KEY_BEGIN = KEY_KP5,
  KEY_UP = 103,
  KEY_DOWN = 108,

#else
  // from ncurses.h
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
  KEY_KP0 = '0', // TODO: verify
  KEY_KP1 = '1',
  KEY_KP2 = '2',
  KEY_KP3 = '3',
  KEY_KP4 = '4',
  KEY_KP5 = '5',
  KEY_KP6 = '6',
  KEY_KP7 = '7',
  KEY_KP8 = '8',
  KEY_KP9 = '9',
  KEY_NUMLOCK,
  KEY_KPENTER =0527,
  KEY_KPCOMMA = '.', // TODO verify
  KEY_KPASTERISK ='*',// TODO verify
  KEY_KPSLASH ='/', // TODO verify
  KEY_KPEQUAL = '=', // TODO verify
  KEY_KPPLUS = '+', // TODO verify
  KEY_KPMINUS = '-', // TODO verify
  KEY_KPHOME = 0534, // KEY_A1
  KEY_KPEND = 0535, // KEY_A3
  KEY_INSERT = 0514, // TODO: verify KEY_IC Insert Character
  KEY_DELETE = 0512, // TODO: verify KEY_DC Delete Character
  KEY_BEGIN = 0536, // KEY_B2
  KEY_PAGEUP = 0535, // KEY_A3
  KEY_PAGEDOWN = 0540,// KEY_C3
#endif
  KEY_APP1,
  KEY_APP2,
  KEY_APP3,
  KEY_APP4,
  KEY_APP5,
  KEY_APP6,
};

#endif
