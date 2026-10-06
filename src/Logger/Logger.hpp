// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LoggerImpl.hpp"
#include "Device/Features.hpp"
#include "system/Path.hpp"
#include "thread/Mutex.hxx"
#include "util/StaticString.hxx"

#include <array>

struct NMEAInfo;
struct ComputerSettings;

class ProtectedTaskManager;

class Logger {
  LoggerImpl logger;
  mutable Mutex lock;

  /**
   * The HFGPS text of each device, taken when the logger starts, for
   * the L record that names a new GPS source (#3180).
   */
  std::array<StaticString<32>, NUMDEV> gps_device_names;

  /** The device the IGC file named last as the GPS source */
  unsigned gps_device = 0;

  void LogEvent(const NMEAInfo &gps_info, const char*);

public:
  void LogPoint(const NMEAInfo &gps_info);
  void LogStartEvent(const NMEAInfo &gps_info);
  void LogFinishEvent(const NMEAInfo &gps_info);
  void LogPilotEvent(const NMEAInfo &gps_info);

  [[gnu::pure]]
  bool IsLoggerActive() const noexcept;

  /**
   * The IGC file being written right now, or nullptr while the
   * logger is off.  A backup leaves it out: it is not sharable on
   * Windows, and incomplete anyway.
   */
  [[gnu::pure]]
  AllocatedPath GetActivePath() const noexcept;

  void GUIStartLogger(const NMEAInfo& gps_info,
                      const ComputerSettings& settings,
                      const ProtectedTaskManager *protected_task_manager,
                      bool noAsk = false);
  void GUIToggleLogger(const NMEAInfo& gps_info,
                       const ComputerSettings& settings,
                       const ProtectedTaskManager *protected_task_manager,
                       bool noAsk = false);
  void GUIStopLogger(const NMEAInfo &gps_info,
                     bool noAsk = false);
  void LoggerNote(const char *text);
  void ClearBuffer() noexcept;
};
