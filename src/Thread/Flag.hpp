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

#ifndef XCSOAR_THREAD_FLAG_HPP
#define XCSOAR_THREAD_FLAG_HPP

#include "Util/NonCopyable.hpp"
#include "Thread/Mutex.hpp"
#include "Compiler.h"

/**
 * A flag which can be read and written in a thread-safe way.
 */
class Flag : private NonCopyable {
  /** this mutex protects the value */
  mutable Mutex mutex;

  bool value;

public:
  Flag(bool _value=false):value(_value) {}

public:
  /**
   * Returns the current value of the flag.
   */
  gcc_pure
  bool Get() const {
    ScopeLock protect(mutex);
    return value;
  }

  bool Test() const {
    return Get();
  }

  /**
   * Determine the value of the flag, and set it before returning.
   */
  bool GetAndSet(bool new_value=true) {
    ScopeLock protect(mutex);
    bool old_value = value;
    value = new_value;
    return old_value;
  }

  /**
   * Determine the value of the flag, and clear it before returning.
   */
  bool GetAndClear() {
    return GetAndSet(false);
  }

  /**
   * Set a new value for the flag.
   */
  void Set(bool _value=true) {
    ScopeLock protect(mutex);
    value = _value;
  }

  void Signal() {
    Set(true);
  }

  void Reset() {
    Set(false);
  }
};

#endif
