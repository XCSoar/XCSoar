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

#ifndef XCSOAR_EVENT_ANDROID_KEY_CODE_HPP
#define XCSOAR_EVENT_ANDROID_KEY_CODE_HPP

/**
 * @see http://developer.android.com/reference/android/view/KeyEvent.html
 */
enum {
  KEY_UP = 0x13,
  KEY_DOWN = 0x14,
  KEY_LEFT = 0x15,
  KEY_RIGHT = 0x16,
  KEY_TAB = 0x3d,
  KEY_SPACE = 0x3e,
  KEY_RETURN = 0x42,
  KEY_MENU = 0x52,
  KEY_ESCAPE = 0x6f,

  KEY_DUMMY = 0x80,
  KEY_BACK,
  KEY_HOME,
  KEY_END,
  KEY_PRIOR,
  KEY_NEXT,
  KEY_F1 = 0x83,
  KEY_F2 = 0x84,
  KEY_F3 = 0x85,
  KEY_F4 = 0x86,
  KEY_F5 = 0x87,
  KEY_F6 = 0x88,
  KEY_F7 = 0x89,
  KEY_F8 = 0x8a,
  KEY_F9 = 0x8b,
  KEY_F10 = 0x8c,
  KEY_F11 = 0x8d,
  KEY_F12 = 0x8e,
  KEY_APP1,
  KEY_APP2,
  KEY_APP3,
  KEY_APP4,
  KEY_APP5,
  KEY_APP6,
};

#endif
