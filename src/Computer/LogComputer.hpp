// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "time/GPSClock.hpp"

#include <cassert>

struct NMEAInfo;
struct MoreData;
struct DerivedInfo;
struct LoggerSettings;
class Logger;

class LogComputer {
  GeoPoint last_location;

  GPSClock log_clock;

  /** number of points to log at high rate */
  unsigned fast_log_num;

  Logger *logger = nullptr;

public:
  void SetLogger(Logger *_logger) noexcept {
    assert(logger == nullptr);
    assert(_logger != nullptr);

    logger = _logger;
  }

  void Reset() noexcept;
  void StartTask(const NMEAInfo &basic) noexcept;
  bool Run(const MoreData &basic, const DerivedInfo &calculated,
           const LoggerSettings &settings_logger) noexcept;

  void SetFastLogging() noexcept {
    fast_log_num = 5;
  }
};
