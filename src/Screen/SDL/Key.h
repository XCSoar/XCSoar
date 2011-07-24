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

#ifndef XCSOAR_SCREEN_SDL_KEY_H
#define XCSOAR_SCREEN_SDL_KEY_H

#include <SDL_keysym.h>

enum {
  VK_SPACE = SDLK_SPACE,
  VK_UP = SDLK_UP,
  VK_DOWN = SDLK_DOWN,
  VK_LEFT = SDLK_LEFT,
  VK_RIGHT = SDLK_RIGHT,
  VK_HOME = SDLK_HOME,
  VK_END = SDLK_END,
  VK_PRIOR = SDLK_PAGEUP,
  VK_NEXT = SDLK_PAGEDOWN,
  VK_RETURN = SDLK_RETURN,
  VK_F1 = SDLK_F1,
  VK_F2 = SDLK_F2,
  VK_F3 = SDLK_F3,
  VK_F4 = SDLK_F4,
  VK_F5 = SDLK_F5,
  VK_F6 = SDLK_F6,
  VK_F7 = SDLK_F7,
  VK_F8 = SDLK_F8,
  VK_F9 = SDLK_F9,
  VK_F10 = SDLK_F10,
  VK_F11 = SDLK_F11,
  VK_F12 = SDLK_F12,
  VK_ESCAPE = SDLK_ESCAPE,
  VK_TAB = SDLK_TAB,
  VK_BACK = SDLK_BACKSPACE,
  VK_MENU = SDLK_MENU,
  VK_APP1,
  VK_APP2,
  VK_APP3,
  VK_APP4,
  VK_APP5,
  VK_APP6,
};

#endif
