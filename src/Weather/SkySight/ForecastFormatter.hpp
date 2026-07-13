// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ForecastUtils.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "util/StaticString.hxx"

#include <chrono>

namespace SkySight {

[[nodiscard]] inline StaticString<32>
FormatForecastTimeLabel(const Layer &layer, time_t forecast_time,
                        RoughTimeDelta utc_offset) noexcept
{
  StaticString<32> label;
  if (forecast_time <= 0)
    return label;

  label = FormatLocalDateTimeYYYYMMDDHHMM(
    TimeStamp(std::chrono::duration<double>(forecast_time)), utc_offset).c_str();

  if (IsFullDayForecastLayer(layer))
    label.Truncate(10);

  return label;
}

} // namespace SkySight
