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

#ifndef XCSOAR_EVENT_LIBINPUT_LIBINPUT_HPP
#define XCSOAR_EVENT_LIBINPUT_LIBINPUT_HPP

#include "event/PipeEvent.hxx"
#include "ui/dim/Size.hpp"

#include <cassert>

struct libinput;
struct libinput_event;
struct libinput_interface;

class UdevContext;

namespace UI {

class EventQueue;

/**
 * A driver for handling libinput events.
 */
class LibInputHandler final {
  EventQueue &queue;

  UdevContext* udev_context = nullptr;

  struct libinput* li = nullptr;
  struct libinput_interface* li_if = nullptr;

  PipeEvent fd;

  double x = -1.0, y = -1.0;

  PixelSize screen_size{0, 0};

  /**
   * The number of pointer input devices, touch screens ans keyboards.
   */
  unsigned n_pointers = 0, n_touch_screens = 0, n_keyboards = 0;

public:
  explicit LibInputHandler(EventQueue &_queue) noexcept;

  ~LibInputHandler() noexcept {
    Close();
  }

  bool Open() noexcept;
  void Close() noexcept;

  void Suspend() noexcept;
  void Resume() noexcept;

  void SetScreenSize(PixelSize _screen_size) noexcept {
    screen_size = _screen_size;

    assert(screen_size.width > 0);
    assert(screen_size.height > 0);

    if (-1.0 == x)
      x = screen_size.width / 2;

    if (-1.0 == y)
      y = screen_size.height / 2;
  }

  unsigned GetX() const noexcept {
    return (unsigned) x;
  }

  unsigned GetY() const noexcept {
    return (unsigned) y;
  }

  bool HasPointer() const noexcept {
    /* in libinput, touch screens don't have
       LIBINPUT_DEVICE_CAP_POINTER, only LIBINPUT_DEVICE_CAP_TOUCH,
       but for XCSoar, HasPointer() is a superset of
       HasTouchScreen() */
    return (n_pointers + n_touch_screens) > 0;
  }

  bool HasTouchScreen() const noexcept {
    return n_touch_screens > 0;
  }

  bool HasKeyboard() const noexcept {
    return n_keyboards > 0;
  }

private:
  int OpenDevice(const char *path, int flags) noexcept;
  void CloseDevice(int fd) noexcept;

  void HandleEvent(struct libinput_event *li_event) noexcept;
  void HandlePendingEvents() noexcept;

  void OnSocketReady(unsigned events) noexcept;
};

} // namespace UI

#endif
