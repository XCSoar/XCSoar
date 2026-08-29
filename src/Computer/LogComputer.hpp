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

  bool suppressed = false;

public:
  void SetLogger(Logger *_logger) noexcept {
    assert(logger == nullptr);
    assert(_logger != nullptr);

    logger = _logger;
  }

  /**
   * Stop writing anything to the pilot's IGC file, for the duration of a
   * Resume sweep.
   *
   * A separate replay idle entry point keeps Run() out of a sweep, but that
   * is not the
   * only route from ProcessGPS to the file: a replayed task start reaches
   * GlideComputer::OnStartTask() and so StartTask(), which writes an E record
   * -- and IGCWriter::LogEvent() appends a B record after it, as the IGC spec
   * requires.  Those records land at the end of the file the sweep is reading,
   * hours out of order, and replaying them warps time backwards and resets the
   * flying state.
   *
   * A flag here rather than SetLogger(nullptr) because that setter is a
   * one-shot: it asserts the logger has not already been set.
   */
  void SetSuppressed(bool _suppressed) noexcept {
    suppressed = _suppressed;
  }

  void Reset() noexcept;
  void StartTask(const NMEAInfo &basic) noexcept;
  bool Run(const MoreData &basic, const DerivedInfo &calculated,
           const LoggerSettings &settings_logger) noexcept;

  void SetFastLogging() noexcept {
    fast_log_num = 5;
  }
};
