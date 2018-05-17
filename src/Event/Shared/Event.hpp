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

#ifndef XCSOAR_EVENT_SHARED_EVENT_HPP
#define XCSOAR_EVENT_SHARED_EVENT_HPP

#include "Screen/Point.hpp"

#include <assert.h>

struct Event {
  enum Type {
    NOP,

#ifdef USE_POLL_EVENT
    CLOSE,
#else
    TIMER,
#endif

    USER,

    CALLBACK,

    KEY_DOWN,
    KEY_UP,
    MOUSE_MOTION,
    MOUSE_DOWN,
    MOUSE_UP,

    /**
     * The mouse wheel has moved.  The vertical mouse wheel's relative
     * value is int(param).
     */
    MOUSE_WHEEL,

#ifdef ANDROID
    POINTER_DOWN,
    POINTER_UP,

    /**
     * The NativeView was resized (e.g. by changing the screen
     * orientation).
     */
    RESIZE,

    /**
     * The Android Activity is being paused, and the OpenGL surface
     * will be destroyed.
     */
    PAUSE,

    /**
     * The Android Activity is being resumed, and the OpenGL surface
     * can be created again.
     */
    RESUME,
#endif

#ifdef USE_X11
    /**
     * The X11 window was resized.
     */
    RESIZE,

    /**
     * Redraw the screen.
     */
    EXPOSE,
#endif
  };

  typedef void (*Callback)(void *ctx);

  Type type;

  unsigned param;

  void *ptr;

  Callback callback;

  PixelPoint point;

#ifdef USE_X11
  unsigned ch;
#else
  bool is_char;
#endif

  Event() = default;
  Event(Type _type):type(_type) {}
  Event(Type _type, unsigned _param):type(_type), param(_param) {}
  Event(Type _type, unsigned _param, void *_ptr)
    :type(_type), param(_param), ptr(_ptr) {}
  Event(Type _type, void *_ptr):type(_type), ptr(_ptr) {}
  Event(Callback _callback, void *_ptr)
    :type(CALLBACK), ptr(_ptr), callback(_callback) {}
  Event(Type _type, PixelPoint _point)
    :type(_type), point(_point) {}

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

  size_t GetCharacterCount() const {
#ifdef USE_X11
    return type == KEY_DOWN && ch != 0;
#else
    return type == KEY_DOWN && is_char;
#endif
  }

  unsigned GetCharacter(size_t characterIdx) const {
#ifdef USE_X11
    assert(characterIdx == 0);
    assert(ch != 0);

    return ch;
#else
    assert(characterIdx == 0);
    assert(1 == GetCharacterCount());
    return param;
#endif
  }

  bool IsMouseDown() const {
    return type == MOUSE_DOWN;
  }

  bool IsMouse() const {
    return IsMouseDown() || type == MOUSE_UP || type == MOUSE_MOTION ||
      type == MOUSE_WHEEL;
  }

  bool IsUserInput() const {
    return IsKey() || IsMouseDown();
  }
};

#endif
