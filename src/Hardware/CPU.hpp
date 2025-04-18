// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef KOBO
#define HAVE_CPU_FREQUENCY
#endif

#ifdef HAVE_CPU_FREQUENCY

void
LockCPU() noexcept;

void
UnlockCPU() noexcept;

#endif

/**
 * This object locks the CPU at a high clock frequency and unlocks it
 * at the end of the scope.
 */
struct ScopeLockCPU {
#ifdef HAVE_CPU_FREQUENCY
  ScopeLockCPU() noexcept {
    LockCPU();
  }

  ~ScopeLockCPU() noexcept {
    UnlockCPU();
  }
#else
#endif
};
