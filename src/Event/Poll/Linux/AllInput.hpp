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

#ifndef XCSOAR_EVENT_ALL_LINUX_INPUT_HPP
#define XCSOAR_EVENT_ALL_LINUX_INPUT_HPP

#include "Input.hpp"
#include "OS/FileDescriptor.hxx"
#include "IO/Async/FileEventHandler.hpp"
#include "Util/StaticString.hxx"

#include <list>

class IOLoop;
class EventQueue;
class MergeMouse;
struct Event;

/**
 * A container class that queries all Linux input devices
 * (/dev/input/event*).  It uses inotify to detect new/stale devices.
 */
class AllLinuxInputDevices final
#ifdef HAVE_INOTIFY
  : private FileEventHandler
#endif
{
  struct Device {
    StaticString<16> name;

    LinuxInputDevice device;

    Device(const char *_name, IOLoop &_io_loop, EventQueue &_queue,
           MergeMouse &_merge)
      :name(_name), device(_io_loop, _queue, _merge) {}
  };

  IOLoop &io_loop;
  EventQueue &queue;
  MergeMouse &merge;

  std::list<Device> devices;

#ifdef HAVE_INOTIFY
  FileDescriptor inotify_fd;
#endif

public:
  explicit AllLinuxInputDevices(IOLoop &_io_loop, EventQueue &_queue,
                                MergeMouse &_merge)
    :io_loop(_io_loop), queue(_queue), merge(_merge)
#ifdef HAVE_INOTIFY
    , inotify_fd(FileDescriptor::Undefined())
#endif
    {}

  ~AllLinuxInputDevices() {
    Close();
  }

  bool Open();
  void Close();

private:
  gcc_pure
  std::list<Device>::iterator FindByName(const char *name);

  gcc_pure
  static bool CheckName(const char *name);

  void Add(const char *name);

#ifdef HAVE_INOTIFY
  void Remove(const char *name);
  void Read();

  /* virtual methods from FileEventHandler */
  bool OnFileEvent(FileDescriptor fd, unsigned mask) override;
#endif
};

#endif
