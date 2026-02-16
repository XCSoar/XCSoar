// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/RoughTime.hpp"
#include "time/Stamp.hpp"
#include "util/StringBuffer.hxx"

#include <tchar.h>

class RoughTimeDelta;

/**
 * Convert the given time of day from UTC to local time and format it
 * to a user-readable string in the form HH:MM.
 *
 * @param time UTC time of day [seconds]
 */
void
FormatLocalTimeHHMM(char *buffer, TimeStamp time,
                    RoughTimeDelta utc_offset) noexcept;

[[gnu::const]]
static inline BasicStringBuffer<char, 8>
FormatLocalTimeHHMM(TimeStamp time,
                    RoughTimeDelta utc_offset) noexcept
{
  BasicStringBuffer<char, 8> buffer;
  FormatLocalTimeHHMM(buffer.data(), time, utc_offset);
  return buffer;
}
