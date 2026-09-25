// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/UTF8.hpp"

#include <SDL3/SDL_events.h>

#include <cassert>
#include <string>

namespace UI {

enum {
  /**
   * A function pointer with a pointer argument gets called.
   */
  EVENT_CALLBACK = SDL_EVENT_USER,
};

struct Event {
  SDL_Event event;

  /* SDL owns text event memory only until the next event pump.  A modal
     dialog can run a nested event loop while dispatching this event. */
  std::string text;

  void CopyText() {
    if (event.type == SDL_EVENT_TEXT_INPUT)
      text = event.text.text;
  }

  bool IsKeyDown() const {
    return event.type == SDL_EVENT_KEY_DOWN;
  }

  bool IsKey() const {
    return IsKeyDown() || event.type == SDL_EVENT_KEY_UP;
  }

  unsigned GetKeyCode() const {
    assert(IsKey());

    return event.key.key;
  }

  size_t GetCharacterCount() const {
    return event.type == SDL_EVENT_TEXT_INPUT ? LengthUTF8(text.c_str()) : 0;
  }

  unsigned GetCharacter(size_t characterIdx) const {
    assert(characterIdx < GetCharacterCount());

    std::pair<unsigned, const char *> next = NextUTF8(text.c_str());
    for (size_t i = 0; i < characterIdx; ++i)
      next = NextUTF8(next.second);
    return next.first;
  }

  bool IsMouseDown() const {
    return event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
  }

  bool IsMouse() const {
    return IsMouseDown() || event.type == SDL_EVENT_MOUSE_BUTTON_UP ||
      event.type == SDL_EVENT_MOUSE_MOTION;
  }

  bool IsUserInput() const {
    return IsKey() || IsMouse();
  }
};

} // namespace UI
