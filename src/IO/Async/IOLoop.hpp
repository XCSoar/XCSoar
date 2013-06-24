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

#ifndef XCSOAR_IO_LOOP_HPP
#define XCSOAR_IO_LOOP_HPP

#include "OS/Poll.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hpp"
#include "FileEventHandler.hpp"

#include <map>

class FileEventHandler;

/**
 * A thread that is used for asynchronous (non-blocking) I/O.
 */
class IOLoop final {
  struct File {
    File *next_ready;

    const int fd;

    unsigned mask, ready_mask;

    FileEventHandler *handler;

    /**
     * Has this object been modified?  i.e. does it need to be
     * synchronised with the #Poll instance?
     */
    bool modified;

    File(int fd):fd(fd) {}

    File(int fd, unsigned mask, FileEventHandler &handler)
      :fd(fd), mask(mask), ready_mask(0),
       handler(&handler), modified(false) {}

    bool operator<(const File &other) const {
      return fd < other.fd;
    }
  };

  Poll poll;

  Mutex mutex;

  /**
   * Used to synchronise removal.  The calling thread waits for this
   * condition, which is triggered when #running becomes false.
   */
  Cond cond;

  std::map<int, File> files;

  bool modified, running;

public:
  static constexpr unsigned READ = Poll::READ;
  static constexpr unsigned WRITE = Poll::WRITE;

  IOLoop():modified(false), running(false) {}

  gcc_pure
  bool IsEmpty() const {
    return files.empty();
  }

  bool IsModified() const {
    return modified;
  }

  /**
   * Add a file descriptor to the I/O loop.
   *
   * This method is not thread-safe, it may only be called from within
   * the thread.
   */
  void Add(int fd, unsigned mask, FileEventHandler &handler);

  /**
   * Remove a file descriptor from the I/O loop.
   *
   * This method is not thread-safe, it may only be called from within
   * the thread.
   */
  void Remove(int fd);

  void Set(int fd, unsigned mask, FileEventHandler &handler) {
    if (mask != 0)
      Add(fd, mask, handler);
    else
      Remove(fd);
  }

  void Lock() {
    mutex.Lock();
  }

  void Unlock() {
    mutex.Unlock();
  }

  void WaitUntilNotRunning() {
    while (running)
      cond.Wait(mutex);
  }

  void Wait(int timeout_ms=-1);
  void Dispatch();

protected:
  /**
   * Synchronise the file list with the #Poll instance.
   */
  void Update();

  /**
   * Collect a linked list of all file descriptors that are "ready".
   */
  File *CollectReady();

  /**
   * Invoke callbacks for the given linked list.
   */
  void HandleReady(File *ready);
};

#endif
