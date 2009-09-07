/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#ifndef XCSOAR_THREAD_MUTEX_HXX
#define XCSOAR_THREAD_MUTEX_HXX

#ifdef HAVE_POSIX
#include <pthread.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/**
 * This class wraps an OS specific mutex.  It is an object which one
 * thread can wait for, and another thread can wake it up.
 */
class Mutex {
#ifdef HAVE_POSIX
  pthread_mutex_t mutex;
#else
  CRITICAL_SECTION handle;
#endif

public:
  /**
   * Initializes the trigger.
   *
   * @param name an application specific name for this trigger
   */
  Mutex() {
#ifdef HAVE_POSIX
    /* the XCSoar code assumes that recursive locking of a Mutex is
       legal */
    pthread_mutexattr_t recursive;
    pthread_mutexattr_init(&recursive);
    pthread_mutexattr_settype(&recursive, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &recursive);
    pthread_mutexattr_destroy(&recursive);
#else
    ::InitializeCriticalSection(&handle);
#endif
  }
  ~Mutex() {
#ifdef HAVE_POSIX
    pthread_mutex_destroy(&mutex);
#else
    ::DeleteCriticalSection(&handle);
#endif
  }
public:
  void Lock() {
#ifdef HAVE_POSIX
    pthread_mutex_lock(&mutex);
#else
    EnterCriticalSection(&handle);
#endif
  };
  void Unlock() {
#ifdef HAVE_POSIX
    pthread_mutex_unlock(&mutex);
#else
    LeaveCriticalSection(&handle);
#endif
  }
};

// JMW testing an easy/clear way of handling mutexes
class ScopeLock {
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
