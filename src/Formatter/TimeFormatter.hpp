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

#ifndef XCSOAR_TIME_FORMATTER_HPP
#define XCSOAR_TIME_FORMATTER_HPP

#include "Util/StringBuffer.hxx"
#include "Compiler.h"

#include <tchar.h>

struct BrokenDateTime;

/**
 * Format a UTC time stamp in ISO 8601 format (YYYY-MM-DDTHH:MM:SSZ).
 */
void
FormatISO8601(char *buffer, const BrokenDateTime &stamp);

#ifdef _UNICODE
/**
 * Format a UTC time stamp in ISO 8601 format (YYYY-MM-DDTHH:MM:SSZ).
 */
void
FormatISO8601(TCHAR *buffer, const BrokenDateTime &stamp);
#endif

void
FormatTime(TCHAR *buffer, double time);

void
FormatTimeLong(TCHAR *buffer, double time);

/**
 * precedes with "-" if time is negative
 * @param buffer returns HHMM
 * @param time input seconds
 */
void FormatSignedTimeHHMM(TCHAR* buffer, int time);

gcc_const
static inline StringBuffer<TCHAR, 8>
FormatSignedTimeHHMM(int time)
{
  StringBuffer<TCHAR, 8> buffer;
  FormatSignedTimeHHMM(buffer.data(), time);
  return buffer;
}

/**
 * sets HHMMSSSmart and SSSmart
 * if hours > 0, returns HHMM in buffer1 and SS in buffer2
 * if hours == 0, returns MMSS in buffer1 and "" in buffer2
 * @param d input seconds
 */
void FormatTimeTwoLines(TCHAR *buffer1, TCHAR *buffer2, int time);

void FormatTimespanSmart(TCHAR *buffer, int timespan,
                         unsigned max_tokens = 1,
                         const TCHAR *separator = _T(" "));

gcc_const
static inline StringBuffer<TCHAR, 64>
FormatTimespanSmart(int timespan, unsigned max_tokens = 1,
                    const TCHAR *separator = _T(" "))
{
  StringBuffer<TCHAR, 64> buffer;
  FormatTimespanSmart(buffer.data(), timespan, max_tokens, separator);
  return buffer;
}

#endif
