// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LocalTimeFormatter.hpp"
#include "TimeFormatter.hpp"
#include "time/LocalTime.hpp"
#include "time/RoughTime.hpp"

void
FormatLocalTimeHHMM(char *buffer, TimeStamp time,
                    RoughTimeDelta utc_offset) noexcept
{
  FormatTimeHHMM(buffer, TimeLocal(time, utc_offset));
}
