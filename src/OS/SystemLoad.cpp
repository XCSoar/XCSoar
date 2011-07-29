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

#include "SystemLoad.hpp"

#ifdef WIN32

#ifndef _WIN32_WCE

#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdint.h>

unsigned SystemLoadCPU()
{
  unsigned retval = (unsigned)-1;

  static unsigned userTime_last= 0;
  static unsigned kernelTime_last= 0;
  static unsigned tick_last = 0;

  uint64_t  creationTime, exitTime, kernelTime, userTime;
  bool ok = ::GetProcessTimes( ::GetCurrentProcess(),
                               (FILETIME*)&creationTime, (FILETIME*)&exitTime,
                               (FILETIME*)&kernelTime, (FILETIME*)&userTime );

  if (ok) {

    unsigned tick = ::GetTickCount();

    userTime /= 10000;
    kernelTime /= 10000;

    if (tick && (tick_last>0)) {
      unsigned dt_user = userTime-userTime_last;
      unsigned dt_kernel = kernelTime-kernelTime_last;
      unsigned dt = tick-tick_last;
      retval = (100*(dt_user+dt_kernel))/dt;
    }

    tick_last = tick;
    userTime_last = userTime;
    kernelTime_last = kernelTime;
  }

  return retval;
}

#else /* WIN32_WCE */

unsigned SystemLoadCPU()
{
  return (unsigned)-1;
}

#endif

#elif defined(__linux__) || defined(ANDROID)

#include <algorithm>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

struct cpu {
  long busy, idle;
};

unsigned
SystemLoadCPU()
{
  int fd = open("/proc/stat", O_RDONLY|O_NOCTTY);
  if (fd < 0)
    return (unsigned)-1;

  char line[256];
  ssize_t nbytes = read(fd, line, sizeof(line) - 1);
  close(fd);
  if (nbytes <= 0)
    return (unsigned)-1;

  line[nbytes] = 0;

  static const unsigned HISTORY_LENGTH = 5;
  static cpu history[HISTORY_LENGTH];

  cpu current;
  long user, nice, system;
  int n = sscanf(line, "cpu  %ld %ld %ld %ld ", &user, &nice, &system,
                 &current.idle);
  if (n != 4)
    return (unsigned)-1;

  current.busy = user + nice + system;

  const cpu last = history[0];
  std::copy(&history[1], &history[HISTORY_LENGTH], &history[0]);
  history[HISTORY_LENGTH - 1] = current;

  if (last.idle == 0)
    /* first run */
    return (unsigned)-1;

  const cpu diff = {
    current.busy - last.busy,
    current.idle - last.idle,
  };

  long total = diff.busy + diff.idle;
  if (total <= 0)
    return (unsigned)-1;

  return (unsigned)(diff.busy * 100 / total);
}

#else /* !WIN32 */

///@todo implement for non-win32
unsigned SystemLoadCPU()
{
  return (unsigned)-1;
}

#endif /* !WIN32 */
