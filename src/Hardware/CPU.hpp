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

/**
 * Max CPU frequency threshold for #IsSlowCPUFrequency / #IsSlowCPU
 * (1.4 GHz in kHz).
 */
static constexpr unsigned SLOW_CPU_MAX_FREQ_KHZ = 1400000;

/**
 * True if \a max_freq_khz is known and at most #SLOW_CPU_MAX_FREQ_KHZ.
 */
[[gnu::const]]
constexpr bool
IsSlowCPUFrequency(unsigned max_freq_khz) noexcept
{
  return max_freq_khz != 0 && max_freq_khz <= SLOW_CPU_MAX_FREQ_KHZ;
}

/**
 * Maximum CPU frequency in kHz from the OS, or 0 if unknown.
 * Cached after the first successful read on Linux.
 */
[[gnu::pure]]
unsigned
GetMaxCPUFrequencyKHz() noexcept;

/**
 * True when the host max CPU frequency is known and ≤ 1.4 GHz
 * (typical Kobo, Raspberry Pi 1–3 / 3B+, Cubieboard-class boards).
 */
[[gnu::pure]]
bool
IsSlowCPU() noexcept;
