/*
Copyright_License {

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

#include "CPU.hpp"

#ifdef HAVE_CPU_FREQUENCY

#include "OS/FileUtil.hpp"

#include <atomic>

static bool
SetCPUFrequencyGovernor(const char *governor)
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
LockCPU()
{
  if (cpu_lock++ == 0)
    SetCPUFrequencyGovernor("performance");
}

void
UnlockCPU()
{
  if (cpu_lock-- == 1)
    SetCPUFrequencyGovernor("powersave");
}

#endif /* HAVE_CPU_FREQUENCY */
