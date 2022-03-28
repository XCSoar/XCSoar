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

#include "../KeyCode.hpp"
#include "Event.hpp"
namespace UI
{
 unsigned ConvertNumPadKeyToCursorKey(unsigned key_code) noexcept
{
  switch (key_code) {
#if defined(USE_POLL_EVENT)||defined(USE_ANDROID)||defined(USE_WINUSER)
  case KEY_KP2:
#if defined(USE_X11)
  case KEY_KPDOWN:
#endif
    return KEY_DOWN;
  case KEY_KP4:
#if defined(USE_X11)
  case KEY_KPLEFT:
#endif
    return KEY_LEFT;
  case KEY_KP6:
#if defined(USE_X11)
  case KEY_KPRIGHT:
#endif
    return KEY_RIGHT;
  case KEY_KP8:
#if defined(USE_X11)
  case KEY_KPUP:
#endif
    return KEY_UP;
#if defined(USE_X11)
  case KEY_KPENTER:
    return KEY_RETURN;
#endif
#endif
#if defined(ANDROID)
  case KEY_KPENTER:
    return KEY_RETURN;
#endif
  default:
    return key_code; // leave key_code unchanged
  }
}
}

