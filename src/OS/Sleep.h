/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_OS_SLEEP_H

#ifdef WIN32

#include <windows.h>

#else /* !WIN32 */

#include <time.h>

static inline void
Sleep(unsigned ms)
{
  const struct timespec ts = {
    static_cast<time_t>(ms / 1000),
    static_cast<long>((ms % 1000L) * 1000000L),
  };

  nanosleep(&ts, nullptr);
}

#endif /* !WIN32 */

#endif
