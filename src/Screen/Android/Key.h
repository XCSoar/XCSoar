/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_ANDROID_KEY_H
#define XCSOAR_SCREEN_ANDROID_KEY_H

/**
 * @see http://developer.android.com/reference/android/view/KeyEvent.html
 */
enum {
  VK_ESCAPE = 0x04,
  VK_UP = 0x13,
  VK_DOWN = 0x14,
  VK_LEFT = 0x15,
  VK_RIGHT = 0x16,
  VK_TAB = 0x3d,
  VK_SPACE = 0x3e,
  VK_RETURN = 0x42,
  VK_MENU = 0x52,

  VK_DUMMY = 0x80,
  VK_BACK,
  VK_HOME,
  VK_END,
  VK_PRIOR,
  VK_NEXT,
  VK_F1,
  VK_F2,
  VK_F3,
  VK_F4,
  VK_F5,
  VK_F6,
  VK_F7,
  VK_F8,
  VK_F9,
  VK_F10,
  VK_F11,
  VK_F12,
  VK_APP1,
  VK_APP2,
  VK_APP3,
  VK_APP4,
  VK_APP5,
  VK_APP6,
};

#endif
