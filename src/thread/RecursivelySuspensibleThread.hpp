// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SuspensibleThread.hpp"

/**
 * A version of SuspensibleThread that can be suspended recursively.
 * It will not be resumed until all calls to Suspend() have been
 * undone with as many calls to Resume().
 */
class RecursivelySuspensibleThread : public SuspensibleThread {
  /**
   * The thread should be suspended as long as this value is non-zero.
   * It is not protected the SuspensibleThread's mutex, because it may
   * only be accessed from one thread.
   */
  unsigned suspend_count;

public:
  RecursivelySuspensibleThread(const char *_name) noexcept
    :SuspensibleThread(_name) {}

  void Start(bool suspended=false);

  void BeginSuspend() noexcept;

  void Suspend() noexcept;

  void Resume() noexcept;
};
