// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LocalTime.hpp"
#include "RoughTime.hpp"
#include "Stamp.hpp"

TimeStamp
TimeLocal(TimeStamp localtime, RoughTimeDelta utc_offset) noexcept
{
  localtime += utc_offset.ToDuration();

  if (localtime.ToDuration().count() < 0)
    localtime += std::chrono::hours{24};

  return localtime;
}
