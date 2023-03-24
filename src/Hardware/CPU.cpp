// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CPU.hpp"

#ifdef HAVE_CPU_FREQUENCY

#include "system/FileUtil.hpp"

#include <atomic>

static bool
SetCPUFrequencyGovernor(const char *governor) noexcept
{
#ifdef __linux__
  return File::WriteExisting(Path("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"),
                             governor);
#else
  return false;
#endif
}

static std::atomic_uint cpu_lock;

void
LockCPU() noexcept
{
  if (cpu_lock++ == 0)
    SetCPUFrequencyGovernor("performance");
}

void
UnlockCPU() noexcept
{
  if (cpu_lock-- == 1)
    SetCPUFrequencyGovernor("powersave");
}

#endif /* HAVE_CPU_FREQUENCY */
