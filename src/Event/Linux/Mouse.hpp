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

#ifndef XCSOAR_EVENT_LINUX_MOUSE_HPP
#define XCSOAR_EVENT_LINUX_MOUSE_HPP

#include "OS/FileDescriptor.hpp"

struct Event;

/**
 * A driver for the Linux mouse (/dev/input/mouse*, /dev/input/mice).
 */
class LinuxMouse {
  unsigned screen_width, screen_height;
  unsigned x, y;
  bool down;

  bool moved, pressed, released;

  FileDescriptor fd;

public:
  LinuxMouse()
    :screen_width(0), screen_height(0),
     x(0), y(0) {}

  ~LinuxMouse() {
    Close();
  }

  void SetScreenSize(unsigned width, unsigned height);

  unsigned GetX() const {
    return x;
  }

  unsigned GetY() const {
    return y;
  }

  bool Open(const char *path="/dev/input/mice");
  void Close();

  bool IsOpen() const {
    return fd.IsDefined();
  }

  gcc_pure
  int GetFD() const {
    return fd.Get();
  }

  void Read();
  Event Generate();
};

#endif
