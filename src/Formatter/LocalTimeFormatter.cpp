// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LocalTimeFormatter.hpp"
#include "TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/LocalTime.hpp"
#include "time/RoughTime.hpp"
#include "util/StringFormat.hpp"

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
  StringFormat(buffer, 17, "%04u-%02u-%02u %02u:%02u",
               date_time.year, date_time.month, date_time.day,
               date_time.hour, date_time.minute);
}
