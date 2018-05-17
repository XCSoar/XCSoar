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

#ifndef XCSOAR_EVENT_SDL_HPP
#define XCSOAR_EVENT_SDL_HPP

#include "Util/UTF8.hpp"

#include <SDL_events.h>

#include <assert.h>

enum {
  /**
   * A "user" event for a #Window.
   */
  EVENT_USER = SDL_USEREVENT,

  /**
   * A function pointer with a pointer argument gets called.
   */
  EVENT_CALLBACK,
};

struct Event {
  SDL_Event event;

  bool IsKeyDown() const {
    return event.type == SDL_KEYDOWN;
  }

  bool IsKey() const {
    return IsKeyDown() || event.type == SDL_KEYUP;
  }

  unsigned GetKeyCode() const {
    assert(IsKey());

    return event.key.keysym.sym;
  }

  size_t GetCharacterCount() const {
    return event.type == SDL_TEXTINPUT && *event.text.text ?
      LengthUTF8(event.text.text) : 0;
  }

  unsigned GetCharacter(size_t characterIdx) const {
    assert(characterIdx < GetCharacterCount());

    std::pair<unsigned, const char *> next = NextUTF8(event.text.text);
    for (size_t i = 0; i < characterIdx; ++i)
      next = NextUTF8(next.second);
    return next.first;
  }

  bool IsMouseDown() const {
    return event.type == SDL_MOUSEBUTTONDOWN;
  }

  bool IsMouse() const {
    return IsMouseDown() || event.type == SDL_MOUSEBUTTONUP ||
      event.type == SDL_MOUSEMOTION;
  }

  bool IsUserInput() const {
    return IsKey() || IsMouse();
  }
};

#endif
