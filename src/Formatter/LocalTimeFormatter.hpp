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

#ifndef XCSOAR_LOCAL_TIME_FORMATTER_HPP
#define XCSOAR_LOCAL_TIME_FORMATTER_HPP

#include "Time/RoughTime.hpp"
#include "Util/StringBuffer.hxx"
#include "Compiler.h"

#include <tchar.h>

class RoughTimeDelta;

/**
 * Convert the given time of day from UTC to local time and format it
 * to a user-readable string in the form HH:MM.
 *
 * @param time UTC time of day [seconds]
 */
void
FormatLocalTimeHHMM(TCHAR *buffer, int time, RoughTimeDelta utc_offset);

gcc_const
static inline StringBuffer<TCHAR, 8>
FormatLocalTimeHHMM(int time, RoughTimeDelta utc_offset)
{
  StringBuffer<TCHAR, 8> buffer;
  FormatLocalTimeHHMM(buffer.data(), time, utc_offset);
  return buffer;
}

#endif
