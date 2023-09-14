// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LogComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Logger/Settings.hpp"
#include "Logger/Logger.hpp"
#include "LogFile.hpp"

void
LogComputer::Reset() noexcept
{
  last_location = GeoPoint::Invalid();
  fast_log_num = 0;
}

void
LogComputer::StartTask(const NMEAInfo &basic) noexcept
try {
  if (logger != NULL)
    logger->LogStartEvent(basic);
} catch (...) {
  LogError(std::current_exception(), "Logger I/O error");
}

bool
LogComputer::Run(const MoreData &basic, const DerivedInfo &calculated,
                 const LoggerSettings &settings_logger) noexcept
try {
  const bool location_jump = basic.location_available &&
    last_location.IsValid() &&
    basic.location.DistanceS(last_location) > 200;

  last_location = basic.GetLocationOrInvalid();

  if (location_jump)
    // prevent bad fixes from being logged
    return false;

  // log points more often in circling mode
  std::chrono::steady_clock::duration period;
  if (fast_log_num) {
    period = std::chrono::seconds(1);
    fast_log_num--;
  } else
    period = calculated.circling
      ? std::chrono::seconds(settings_logger.time_step_circling)
      : std::chrono::seconds(settings_logger.time_step_cruise);

  if (log_clock.CheckAdvance(basic.time, period) && logger != nullptr)
      logger->LogPoint(basic);

  return true;
} catch (...) {
  LogError(std::current_exception(), "Logger I/O error");
  return false;
}
