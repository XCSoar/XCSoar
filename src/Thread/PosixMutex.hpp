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

#ifndef XCSOAR_THREAD_POSIX_MUTEX_HXX
#define XCSOAR_THREAD_POSIX_MUTEX_HXX

#include "Util/NonCopyable.hpp"

#include <pthread.h>

/**
 * Low-level wrapper for a pthread_mutex_t.
 */
class PosixMutex : private NonCopyable {
  pthread_mutex_t mutex;

  friend class Cond;

public:
  /**
   * Create a "fast" mutex.
   */
  PosixMutex() {
    pthread_mutex_init(&mutex, NULL);
  }

  /**
   * Create a mutex of the specified kind.
   */
  PosixMutex(int kind) {
    pthread_mutexattr_t recursive;
    pthread_mutexattr_init(&recursive);
    pthread_mutexattr_settype(&recursive, kind);
    pthread_mutex_init(&mutex, &recursive);
    pthread_mutexattr_destroy(&recursive);
  }

  ~PosixMutex() {
    pthread_mutex_destroy(&mutex);
  }

  void Lock() {
    pthread_mutex_lock(&mutex);
  }

  bool TryLock() {
    return pthread_mutex_trylock(&mutex) == 0;
  }

  void Unlock() {
    pthread_mutex_unlock(&mutex);
  }
};

#endif
