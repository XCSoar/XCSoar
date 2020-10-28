/* Copyright_License {

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

#ifndef XCSOAR_THREAD_GUARD_HPP
#define XCSOAR_THREAD_GUARD_HPP

#include "SharedMutex.hpp"

/**
 * This class protects its value with a mutex.  A user may get a lease
 * on the value, and the Lease objects locks the mutex during the
 * access.
 */
template<typename T>
class Guard {
protected:
  T &value;
  mutable SharedMutex mutex;

public:
  /**
   * A read-only lease on the guarded value.
   */
  class Lease {
    const Guard &guard;

  public:
    explicit Lease(const Guard &_guard) noexcept:guard(_guard) {
      guard.mutex.lock_shared();
    }

    Lease(const Lease &) = delete;

    ~Lease() noexcept {
      guard.mutex.unlock_shared();
    }

    operator const T&() const noexcept {
      return guard.value;
    }

    const T *operator->() const noexcept {
      return &guard.value;
    }
  };

  /**
   * A writable lease on the guarded value.
   */
  class ExclusiveLease {
    Guard &guard;

  public:
    explicit ExclusiveLease(Guard &_guard) noexcept:guard(_guard) {
      guard.mutex.lock();
    }

    ExclusiveLease(const ExclusiveLease &) = delete;

    ~ExclusiveLease() noexcept {
      guard.mutex.unlock();
    }

    operator const T&() const noexcept {
      return guard.value;
    }

    operator T&() noexcept {
      return guard.value;
    }

    const T *operator->() const noexcept {
      return &guard.value;
    }

    T *operator->() noexcept {
      return &guard.value;
    }
  };

  /**
   * An unprotected writable lease on the value.  This is only allowed
   * when the other threads which might access the value are suspended
   * (e.g. during startup).
   */
  class UnprotectedLease {
    T &value;

  public:
    explicit UnprotectedLease(Guard &_guard) noexcept
      :value(_guard.value) {}

    UnprotectedLease(const UnprotectedLease &) = delete;

    operator const T&() const noexcept {
      return value;
    }

    operator T&() noexcept {
      return value;
    }

    const T *operator->() const noexcept {
      return &value;
    }

    T *operator->() noexcept{
      return &value;
    }
  };

public:
  explicit Guard(T &_value) noexcept:value(_value) {}
};

#endif
