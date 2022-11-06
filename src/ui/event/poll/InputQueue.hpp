/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#ifdef KOBO
#include "linux/MergeMouse.hpp"
#include "linux/Input.hpp"
#else
#include "libinput/LibInputHandler.hpp"
#endif

#include "ui/dim/Point.hpp"

#include <cstdint>

enum class DisplayOrientation : uint8_t;

namespace UI {

class EventQueue;
struct Event;

class InputEventQueue final {
#ifdef KOBO
  MergeMouse merge_mouse;
  LinuxInputDevice keyboard;
  LinuxInputDevice mouse;
#else
  LibInputHandler libinput_handler;
#endif /* !USE_LIBINPUT */

public:
  explicit InputEventQueue(EventQueue &queue) noexcept;
  ~InputEventQueue() noexcept;

  void SetScreenSize(PixelSize screen_size) noexcept {
  #ifdef USE_LIBINPUT
    libinput_handler.SetScreenSize(screen_size);
  #else
    merge_mouse.SetScreenSize(screen_size);
  #endif
  }

#ifndef USE_LIBINPUT
  void SetDisplayOrientation(DisplayOrientation orientation) {
    merge_mouse.SetDisplayOrientation(orientation);
  }
#endif

  bool HasPointer() const noexcept {
#ifdef USE_LIBINPUT
    return libinput_handler.HasPointer();
#else
    return merge_mouse.HasPointer();
#endif
  }

#ifdef USE_LIBINPUT
  bool HasTouchScreen() const noexcept {
    return libinput_handler.HasTouchScreen();
  }

  bool HasKeyboard() const noexcept {
    return libinput_handler.HasKeyboard();
  }
#endif

  PixelPoint GetMousePosition() const noexcept {
#ifdef USE_LIBINPUT
    return PixelPoint(libinput_handler.GetX(), libinput_handler.GetY());
#else
    return merge_mouse.GetPosition();
#endif
  }

  void Suspend() noexcept {
#ifdef USE_LIBINPUT
    libinput_handler.Suspend();
#endif
  }

  void Resume() noexcept {
#ifdef USE_LIBINPUT
    libinput_handler.Resume();
#endif
  }

  bool Generate(Event &event) noexcept;
};

} // namespace UI
