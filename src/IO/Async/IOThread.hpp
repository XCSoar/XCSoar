/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_IO_THREAD_HPP
#define XCSOAR_IO_THREAD_HPP

#include "OS/EventPipe.hpp"
#include "Thread/Thread.hpp"
#include "IOLoop.hpp"

#include <map>

class FileEventHandler;

/**
 * A thread that is used for asynchronous (non-blocking) I/O.
 */
class IOThread final : protected Thread, private FileEventHandler {
  IOLoop loop;

  EventPipe pipe;

  bool quit;

public:
  static constexpr unsigned READ = IOLoop::READ;
  static constexpr unsigned WRITE = IOLoop::WRITE;

  IOThread():Thread("IOThread") {}

  /**
   * Start the thread.  This method should be called after creating
   * this object.
   */
  bool Start();

  /**
   * Stop the thread.  This method must be called before the
   * destructor.
   */
  void Stop();

  /**
   * Add a file descriptor to the I/O loop.
   *
   * This method is not thread-safe, it may only be called from within
   * the thread.
   */
  void Add(FileDescriptor fd, unsigned mask, FileEventHandler &handler) {
    loop.Add(fd, mask, handler);
  }

  /**
   * Remove a file descriptor from the I/O loop.
   *
   * This method is not thread-safe, it may only be called from within
   * the thread.
   */
  void Remove(FileDescriptor fd) {
    loop.Remove(fd);
  }

  void Set(FileDescriptor fd, unsigned mask, FileEventHandler &handler) {
    loop.Set(fd, mask, handler);
  }

  /**
   * A thread-safe version of Add().
   */
  void LockAdd(FileDescriptor fd, unsigned mask, FileEventHandler &handler);

  /**
   * A thread-safe version of Remove().
   *
   * This method is synchronous: after this call, the handler is
   * guaranteed to be not running.
   */
  void LockRemove(FileDescriptor fd);

  /**
   * A thread-safe version of Set().
   */
  void LockSet(FileDescriptor fd, unsigned mask, FileEventHandler &handler) {
    if (mask != 0)
      LockAdd(fd, mask, handler);
    else
      LockRemove(fd);
  }

protected:
  /* virtual methods from Thread */
  void Run() override;

  /* virtual methods from FileEventHandler */
  bool OnFileEvent(FileDescriptor fd, unsigned mask) override;
};

#endif
