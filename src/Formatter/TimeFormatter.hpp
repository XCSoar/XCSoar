/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "time/Stamp.hpp"
#include "util/StringBuffer.hxx"

#include <tchar.h>

struct BrokenDate;
struct BrokenDateTime;

void
FormatISO8601(char *buffer, const BrokenDate &date) noexcept;

#ifdef _UNICODE
void
FormatISO8601(TCHAR *buffer, const BrokenDate &date) noexcept;
#endif

/**
 * Format a UTC time stamp in ISO 8601 format (YYYY-MM-DDTHH:MM:SSZ).
 */
void
FormatISO8601(char *buffer, const BrokenDateTime &stamp) noexcept;

#ifdef _UNICODE
/**
 * Format a UTC time stamp in ISO 8601 format (YYYY-MM-DDTHH:MM:SSZ).
 */
void
FormatISO8601(TCHAR *buffer, const BrokenDateTime &stamp) noexcept;
#endif

void
FormatTime(TCHAR *buffer, FloatDuration time) noexcept;

static inline void
FormatTime(TCHAR *buffer, TimeStamp time) noexcept
{
  FormatTime(buffer, time.ToDuration());
}

void
FormatTimeLong(TCHAR *buffer, FloatDuration time) noexcept;

/**
 * precedes with "-" if time is negative
 * @param buffer returns HHMM
 * @param time input seconds
 */
void
FormatSignedTimeHHMM(TCHAR *buffer, std::chrono::seconds time) noexcept;

[[gnu::const]]
static inline BasicStringBuffer<TCHAR, 8>
FormatSignedTimeHHMM(std::chrono::seconds time) noexcept
{
  BasicStringBuffer<TCHAR, 8> buffer;
  FormatSignedTimeHHMM(buffer.data(), time);
  return buffer;
}

[[gnu::const]]
static inline auto
FormatSignedTimeHHMM(FloatDuration time) noexcept
{
  return FormatSignedTimeHHMM(std::chrono::duration_cast<std::chrono::seconds>(time));
}

static inline void
FormatTimeHHMM(TCHAR *buffer, TimeStamp time) noexcept
{
  FormatSignedTimeHHMM(buffer, time.Cast<std::chrono::seconds>());
}

[[gnu::const]]
static inline auto
FormatTimeHHMM(std::chrono::duration<unsigned> time) noexcept
{
  return FormatSignedTimeHHMM(std::chrono::duration_cast<std::chrono::seconds>(time));
}

[[gnu::const]]
static inline auto
FormatTimeHHMM(TimeStamp time) noexcept
{
  return FormatSignedTimeHHMM(time.Cast<std::chrono::seconds>());
}

/**
 * sets HHMMSSSmart and SSSmart
 * if hours > 0, returns HHMM in buffer1 and SS in buffer2
 * if hours == 0, returns MMSS in buffer1 and "" in buffer2
 * @param d input seconds
 */
void
FormatTimeTwoLines(TCHAR *buffer1, TCHAR *buffer2,
                   std::chrono::seconds time) noexcept;

void
FormatTimespanSmart(TCHAR *buffer, std::chrono::seconds timespan,
                    unsigned max_tokens = 1,
                    const TCHAR *separator = _T(" ")) noexcept;

[[gnu::const]]
static inline BasicStringBuffer<TCHAR, 64>
FormatTimespanSmart(std::chrono::seconds timespan, unsigned max_tokens = 1,
                    const TCHAR *separator = _T(" ")) noexcept
{
  BasicStringBuffer<TCHAR, 64> buffer;
  FormatTimespanSmart(buffer.data(), timespan, max_tokens, separator);
  return buffer;
}

[[gnu::const]]
static inline auto
FormatTimespanSmart(FloatDuration timespan, unsigned max_tokens = 1,
                    const TCHAR *separator = _T(" ")) noexcept
{
  return FormatTimespanSmart(std::chrono::duration_cast<std::chrono::seconds>(timespan),
                             max_tokens, separator);
}

#endif
