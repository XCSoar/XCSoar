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

#ifndef XCSOAR_EVENT_LINUX_INPUT_HPP
#define XCSOAR_EVENT_LINUX_INPUT_HPP

#include "event/SocketEvent.hxx"
#include "Math/Point2D.hpp"

namespace UI {

class EventQueue;
class MergeMouse;
struct Event;

/**
 * A driver for Linux input devices (/dev/input/event*).
 */
class LinuxInputDevice final {
  typedef IntPoint2D Position;

  EventQueue &queue;

  MergeMouse &merge;

  int min_x, max_x, min_y, max_y;

  /**
   * The position being edited.  Upon EV_SYN, it will be copied to
   * #moved_position if #moving is true.
   */
  Position edit_position;

  /**
   * The position published by Generate().
   */
  Position public_position;

  int rel_x, rel_y, rel_wheel;

  bool down;

  /**
   * Was #edit_position modified?
   */
  bool moving;

  /**
   * Was the finger pressed or released, but not yet committed with
   * EV_SYN/SYN_REPORT?
   */
  bool pressing, releasing;

  bool is_pointer;

  SocketEvent socket_event;

public:
  LinuxInputDevice(EventQueue &_queue, MergeMouse &_merge);

  ~LinuxInputDevice() {
    Close();
  }

  bool Open(const char *path);
  void Close();

  bool IsOpen() const {
    return socket_event.IsDefined();
  }

private:
  void Read();

  void OnSocketReady(unsigned events) noexcept;
};

} // namespace UI

#endif
