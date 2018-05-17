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

#ifndef XCSOAR_EVENT_SDL_KEY_CODE_HPP
#define XCSOAR_EVENT_SDL_KEY_CODE_HPP

#include <SDL_keyboard.h>

enum {
  KEY_SPACE = SDLK_SPACE,
  KEY_UP = SDLK_UP,
  KEY_DOWN = SDLK_DOWN,
  KEY_LEFT = SDLK_LEFT,
  KEY_RIGHT = SDLK_RIGHT,
  KEY_HOME = SDLK_HOME,
  KEY_END = SDLK_END,
  KEY_PRIOR = SDLK_PAGEUP,
  KEY_NEXT = SDLK_PAGEDOWN,
  KEY_RETURN = SDLK_RETURN,
  KEY_F1 = SDLK_F1,
  KEY_F2 = SDLK_F2,
  KEY_F3 = SDLK_F3,
  KEY_F4 = SDLK_F4,
  KEY_F5 = SDLK_F5,
  KEY_F6 = SDLK_F6,
  KEY_F7 = SDLK_F7,
  KEY_F8 = SDLK_F8,
  KEY_F9 = SDLK_F9,
  KEY_F10 = SDLK_F10,
  KEY_F11 = SDLK_F11,
  KEY_F12 = SDLK_F12,
  KEY_ESCAPE = SDLK_ESCAPE,
  KEY_TAB = SDLK_TAB,
  KEY_BACK = SDLK_BACKSPACE,
  KEY_MENU = SDLK_MENU,
  KEY_APP1,
  KEY_APP2,
  KEY_APP3,
  KEY_APP4,
  KEY_APP5,
  KEY_APP6,
};

#endif
