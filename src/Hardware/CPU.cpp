// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CPU.hpp"
#include "system/FileUtil.hpp"
#include "util/NumberParser.hpp"
#include "util/StringStrip.hxx"

#include <atomic>

#ifdef HAVE_CPU_FREQUENCY

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

static unsigned
ReadMaxCPUFrequencyKHz() noexcept
{
#ifdef __linux__
  char buffer[64];
  if (!File::ReadString(Path("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"),
                        buffer, sizeof(buffer)))
    return 0;

  StripRight(buffer);

  char *endptr;
  const unsigned value = ParseUnsigned(buffer, &endptr, 10);
  if (endptr == buffer || *endptr != '\0' || value == 0)
    return 0;

  return value;
#else
  return 0;
#endif
}

unsigned
GetMaxCPUFrequencyKHz() noexcept
{
  static const unsigned cached = ReadMaxCPUFrequencyKHz();
  return cached;
}

bool
IsSlowCPU() noexcept
{
  return IsSlowCPUFrequency(GetMaxCPUFrequencyKHz());
}
