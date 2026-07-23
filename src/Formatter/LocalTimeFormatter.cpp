// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LocalTimeFormatter.hpp"
#include "TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/LocalTime.hpp"
#include "time/RoughTime.hpp"

#include <chrono>

void
FormatLocalTimeHHMM(char *buffer, TimeStamp time,
                    RoughTimeDelta utc_offset) noexcept
{
  FormatTimeHHMM(buffer, TimeLocal(time, utc_offset));
}

void
FormatLocalDateTimeYYYYMMDDHHMM(char *buffer, TimeStamp time,
                                RoughTimeDelta utc_offset) noexcept
{
  const auto duration = std::chrono::duration_cast<std::chrono::system_clock::duration>(
    TimeLocal(time, utc_offset).ToDuration());
  const BrokenDateTime date_time{std::chrono::system_clock::time_point{duration}};
  FormatISO8601(buffer, date_time);
  buffer[10] = ' ';
  buffer[16] = 0;
}
