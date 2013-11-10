/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "OS/FileDescriptor.hpp"
#include "IO/Async/FileEventHandler.hpp"

class EventQueue;
class IOLoop;
struct Event;

/**
 * A driver for Linux input devices (/dev/input/event*).
 */
class LinuxInputDevice final : private FileEventHandler {
  struct Position {
    unsigned x, y;

    static constexpr Position Zero() {
      return { 0, 0 };
    }
  };

  EventQueue &queue;
  IOLoop &io_loop;

  /**
   * The position being edited.  Upon EV_SYN, it will be copied to
   * #moved_position if #moving is true.
   */
  Position edit_position;

  /**
   * The position published by Generate().
   */
  Position public_position;

  bool down;

  /**
   * Was #edit_position modified?
   */
  bool moving;

  /**
   * Does #public_position contain a new value?  This is copied from
   * #moving on EV_SYN and will be picked up by Generate().
   */
  bool moved;

  bool pressed, released;

  FileDescriptor fd;

public:
  explicit LinuxInputDevice(EventQueue &_queue, IOLoop &_io_loop)
    :queue(_queue), io_loop(_io_loop),
     edit_position(Position::Zero()), public_position(Position::Zero()) {}

  ~LinuxInputDevice() {
    Close();
  }

  bool Open(const char *path);
  void Close();

  bool IsOpen() const {
    return fd.IsDefined();
  }

  Event Generate();

private:
  void Read();

  /* virtual methods from FileEventHandler */
  virtual bool OnFileEvent(int fd, unsigned mask) override;
};

#endif
