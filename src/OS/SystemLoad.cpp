/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#else /* !WIN32 */

///@todo implement for non-win32
unsigned SystemLoadCPU()
{
  return (unsigned)-1;
}

#endif /* !WIN32 */
