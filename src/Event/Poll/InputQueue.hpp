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

#ifndef XCSOAR_EVENT_POLL_INPUT_QUEUE_HPP
#define XCSOAR_EVENT_POLL_INPUT_QUEUE_HPP

#ifdef USE_LIBINPUT
#include "LibInput/LibInputHandler.hpp"
#else
#include "Linux/MergeMouse.hpp"
#ifdef KOBO
#include "Linux/Input.hpp"
#elif defined(USE_LINUX_INPUT)
#include "Linux/AllInput.hpp"
#else
#include "Linux/TTYKeyboard.hpp"
#include "Linux/Mouse.hpp"
#endif
#endif

#include "Screen/Point.hpp"

#include <stdint.h>

enum class DisplayOrientation : uint8_t;
class IOLoop;
class EventQueue;
struct Event;

class InputEventQueue final {
#ifdef USE_LIBINPUT
  LibInputHandler libinput_handler;
#else /* !USE_LIBINPUT */
  MergeMouse merge_mouse;
#ifdef KOBO
  LinuxInputDevice keyboard;
  LinuxInputDevice mouse;
#elif defined(USE_LINUX_INPUT)
  AllLinuxInputDevices all_input;
#else
  TTYKeyboard keyboard;
  LinuxMouse mouse;
#endif
#endif /* !USE_LIBINPUT */

public:
  InputEventQueue(IOLoop &io_loop, EventQueue &queue);
  ~InputEventQueue();

  void SetScreenSize(unsigned width, unsigned height) {
  #ifdef USE_LIBINPUT
    libinput_handler.SetScreenSize(width, height);
  #else
    merge_mouse.SetScreenSize(width, height);
  #endif
  }

#ifndef USE_LIBINPUT
  void SetMouseRotation(bool swap, bool invert_x, bool invert_y) {
    merge_mouse.SetSwap(swap);
    merge_mouse.SetInvert(invert_x, invert_y);
  }

  void SetMouseRotation(DisplayOrientation orientation);
#endif

  bool HasPointer() const {
#ifdef USE_LIBINPUT
    return libinput_handler.HasPointer();
#else
    return merge_mouse.HasPointer();
#endif
  }

#ifdef USE_LIBINPUT
  bool HasTouchScreen() const {
    return libinput_handler.HasTouchScreen();
  }

  bool HasKeyboard() const {
    return libinput_handler.HasKeyboard();
  }
#endif

  RasterPoint GetMousePosition() const {
#ifdef USE_LIBINPUT
    return { int(libinput_handler.GetX()), int(libinput_handler.GetY()) };
#else
    return { int(merge_mouse.GetX()), int(merge_mouse.GetY()) };
#endif
  }

  bool Generate(Event &event);
};

#endif
