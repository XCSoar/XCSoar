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

#ifndef XCSOAR_EVENT_LINUX_MOUSE_HPP
#define XCSOAR_EVENT_LINUX_MOUSE_HPP

#include "OS/FileDescriptor.hxx"
#include "IO/Async/FileEventHandler.hpp"

class IOLoop;
class MergeMouse;

/**
 * A driver for the Linux mouse (/dev/input/mouse*, /dev/input/mice).
 */
class LinuxMouse final : private FileEventHandler {
  IOLoop &io_loop;

  MergeMouse &merge;

  FileDescriptor fd;

public:
  explicit LinuxMouse(IOLoop &_io_loop, MergeMouse &_merge)
    :io_loop(_io_loop), merge(_merge), fd(FileDescriptor::Undefined()) {}

  ~LinuxMouse() {
    Close();
  }

  bool Open(const char *path="/dev/input/mice");
  void Close();

  bool IsOpen() const {
    return fd.IsDefined();
  }

private:
  void Read();

  /* virtual methods from FileEventHandler */
  bool OnFileEvent(FileDescriptor fd, unsigned mask) override;
};

#endif
