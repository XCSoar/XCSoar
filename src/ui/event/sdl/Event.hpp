// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/UTF8.hpp"

#include <SDL_events.h>

#include <cassert>

namespace UI {

enum {
  /**
   * A function pointer with a pointer argument gets called.
   */
  EVENT_CALLBACK = SDL_USEREVENT,
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

} // namespace UI
