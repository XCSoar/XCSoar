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

#include "Thread/FastMutex.hpp"

#include <assert.h>

#ifndef NDEBUG
#include "Thread/Handle.hpp"
#endif

/**
 * This class wraps an OS specific mutex.  It is an object which one
 * thread can wait for, and another thread can wake it up.
 */
class Mutex {
  FastMutex mutex;

#ifndef NDEBUG
  /**
   * Protect the attributes "locked" and "owner".
   */
  mutable FastMutex debug_mutex;

  bool locked;
  ThreadHandle owner;
#endif

#ifdef HAVE_POSIX
  friend class PosixCond;
#else
  friend class WindowsCond;
#endif
  friend class TemporaryUnlock;

public:
  /**
   * Initializes the Mutex
   */
#if defined(HAVE_POSIX) && defined(NDEBUG) && defined(__GLIBC__)
  constexpr
#endif
  Mutex()
#ifndef NDEBUG
    :locked(false)
#endif
  {
  }

#ifndef NDEBUG
  /**
   * Deletes the Mutex
   */
  ~Mutex() {
    assert(!locked);
  }
#endif

public:
#ifndef NDEBUG
  /**
   * Determine if the current thread has locked this mutex.  This is a
   * debug-only method meant for assert() calls, and is not available
   * in optimised builds.
   */
  gcc_pure
  bool IsLockedByCurrent() const {
    debug_mutex.lock();
    bool result = locked && owner.IsInside();
    debug_mutex.unlock();
    return result;
  }
#endif

  /**
   * Locks the Mutex
   */
  void Lock() {
#ifdef NDEBUG
    mutex.lock();
#else
    if (!mutex.try_lock()) {
      /* locking has failed - at this point, "locked" and "owner" are
         either not yet update, or "owner" is set to another thread */
      assert(!IsLockedByCurrent());

      mutex.lock();
    }

    /* we have just obtained the mutex; the "locked" flag must not be
       set */
    debug_mutex.lock();
    assert(!locked);
    locked = true;
    owner = ThreadHandle::GetCurrent();
    debug_mutex.unlock();
#endif
  };

  /**
   * Tries to lock the Mutex
   */
  bool TryLock() {
    if (!mutex.try_lock()) {
#ifndef NDEBUG
      assert(!IsLockedByCurrent());
#endif
      return false;
    }

#ifndef NDEBUG
    debug_mutex.lock();
    assert(!locked);
    locked = true;
    owner = ThreadHandle::GetCurrent();
    debug_mutex.unlock();
#endif
    return true;
  };

  /**
   * Unlocks the Mutex
   */
  void Unlock() {
#ifndef NDEBUG
    debug_mutex.lock();
    assert(locked);
    assert(owner.IsInside());
    locked = false;
    debug_mutex.unlock();
#endif

    mutex.unlock();
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
class ScopeLock {
  Mutex &scope_mutex;

public:
  ScopeLock(Mutex& the_mutex):scope_mutex(the_mutex) {
    scope_mutex.Lock();
  };

  ~ScopeLock() {
    scope_mutex.Unlock();
  }

  ScopeLock(const ScopeLock &other) = delete;
  ScopeLock &operator=(const ScopeLock &other) = delete;
};

/**
 * Within the scope of an instance, this class will keep a #Mutex
 * unlocked.
 */
class ScopeUnlock {
  Mutex &mutex;

public:
  explicit ScopeUnlock(Mutex &_mutex):mutex(_mutex) {
    mutex.Unlock();
  };

  ~ScopeUnlock() {
    mutex.Lock();
  }

  ScopeUnlock(const ScopeUnlock &other) = delete;
  ScopeUnlock &operator=(const ScopeUnlock &other) = delete;
};

/**
 * A debug-only class that changes internal debug flags to indicate
 * "not locked", even though you did lock it.  An instance of this
 * class shall wrap function calls that will temporarily unlock the
 * mutex, such as pthread_cond_wait().
 */
class TemporaryUnlock {
#ifndef NDEBUG
  Mutex &mutex;

public:
  TemporaryUnlock(Mutex &_mutex):mutex(_mutex) {
    mutex.debug_mutex.lock();
    assert(mutex.locked);
    assert(mutex.owner.IsInside());
    mutex.locked = false;
    mutex.debug_mutex.unlock();
  }

  ~TemporaryUnlock() {
    mutex.debug_mutex.lock();
    assert(!mutex.locked);
    mutex.owner = ThreadHandle::GetCurrent();
    mutex.locked = true;
    mutex.debug_mutex.unlock();
  }
#else
public:
  constexpr TemporaryUnlock(Mutex &_mutex) {}
#endif

  TemporaryUnlock(const TemporaryUnlock &other) = delete;
  TemporaryUnlock &operator=(const TemporaryUnlock &other) = delete;
};

#endif
