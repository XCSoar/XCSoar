/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_EVENT_EGL_EVENT_HPP
#define XCSOAR_EVENT_EGL_EVENT_HPP

#include "Screen/Point.hpp"

#include <assert.h>

struct Event {
  enum Type {
    NOP,
    QUIT,
    CLOSE,
    TIMER,
    USER,

    CALLBACK,

    KEY_DOWN,
    KEY_UP,
    MOUSE_MOTION,
    MOUSE_DOWN,
    MOUSE_UP,
  };

  typedef void (*Callback)(void *ctx);

  Type type;

  unsigned param;

  void *ptr;

  Callback callback;

  PixelScalar x, y;

  Event() = default;
  Event(Type _type):type(_type) {}
  Event(Type _type, unsigned _param):type(_type), param(_param) {}
  Event(Type _type, unsigned _param, void *_ptr)
    :type(_type), param(_param), ptr(_ptr) {}
  Event(Type _type, void *_ptr):type(_type), ptr(_ptr) {}
  Event(Callback _callback, void *_ptr)
    :type(CALLBACK), ptr(_ptr), callback(_callback) {}
  Event(Type _type, PixelScalar _x, PixelScalar _y)
    :type(_type), x(_x), y(_y) {}

  constexpr
  RasterPoint GetPoint() const {
    return RasterPoint{x, y};
  }

  bool IsKeyDown() const {
    return type == KEY_DOWN;
  }

  bool IsKey() const {
    return IsKeyDown() || type == KEY_UP;
  }

  unsigned GetKeyCode() const {
    assert(IsKey());

    return param;
  }

  bool IsCharacter() const {
    return false;
  }

  unsigned GetCharacter() const {
    assert(false);
    return 0;
  }

  bool IsMouseDown() const {
    return type == MOUSE_DOWN;
  }

  bool IsMouse() const {
    return IsMouseDown() || type == MOUSE_UP || type == MOUSE_MOTION;
  }

  bool IsUserInput() const {
    return IsKey() || IsMouseDown();
  }
};

#endif
