// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
