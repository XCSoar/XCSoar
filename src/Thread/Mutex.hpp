/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

/**
 * This file is about mutexes.
 *
 * A mutex lock is also known as a mutually exclusive lock. This type of lock
 * is provided by many threading systems as a means of synchronization.
 * Basically, it is only possible for one thread to grab a mutex at a time:
 * if two threads try to grab a mutex, only one succeeds. The other thread
 * has to wait until the first thread releases the lock; it can then grab the
 * lock and continue operation.
 * @file Mutex.hpp
 */

#ifndef XCSOAR_THREAD_MUTEX_HXX
#define XCSOAR_THREAD_MUTEX_HXX

#include "Util/NonCopyable.hpp"
#include "Thread/RecursiveMutex.hpp"

#ifndef NDEBUG
#include "Thread/Local.hpp"
extern ThreadLocalInteger thread_locks_held;
#endif

/**
 * This class wraps an OS specific mutex.  It is an object which one
 * thread can wait for, and another thread can wake it up.
 */
class Mutex : private NonCopyable {
  RecursiveMutex mutex;

  friend class Cond;

public:
  /**
   * Initializes the Mutex
   */
  Mutex() {
  }

  /**
   * Deletes the Mutex
   */
  ~Mutex() {
  }

public:
  /**
   * Locks the Mutex
   */
  void Lock() {
    mutex.Lock();

#ifndef NDEBUG
    ++thread_locks_held;
#endif
  };

  /**
   * Tries to lock the Mutex
   */
  bool TryLock() {
    if (!mutex.TryLock())
      return false;

#ifndef NDEBUG
    ++thread_locks_held;
#endif
    return true;
  };

  /**
   * Unlocks the Mutex
   */
  void Unlock() {
    mutex.Unlock();

#ifndef NDEBUG
    --thread_locks_held;
#endif
  }
};

/**
 * A class for an easy and clear way of handling mutexes
 *
 * Creating a ScopeLock instance locks the given Mutex, while
 * destruction of the ScopeLock leads to unlocking the Mutex.
 *
 * Usage: Create a ScopeLock at the beginning of a function and
 * after function is executed the ScopeLock will destroy itself
 * and unlock the Mutex again.
 * @author JMW
 */
class ScopeLock : private NonCopyable {
public:
  ScopeLock(Mutex& the_mutex):scope_mutex(the_mutex) {
    scope_mutex.Lock();
  };
  ~ScopeLock() {
    scope_mutex.Unlock();
  }
private:
  Mutex &scope_mutex;
};

#endif
