// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LoggerImpl.hpp"
#include "thread/Mutex.hxx"

#include <tchar.h>

struct NMEAInfo;
struct ComputerSettings;

class ProtectedTaskManager;

class Logger {
  LoggerImpl logger;
  mutable Mutex lock;

  void LogEvent(const NMEAInfo &gps_info, const char*);

public:
  void LogPoint(const NMEAInfo &gps_info);
  void LogStartEvent(const NMEAInfo &gps_info);
  void LogFinishEvent(const NMEAInfo &gps_info);
  void LogPilotEvent(const NMEAInfo &gps_info);

  [[gnu::pure]]
  bool IsLoggerActive() const noexcept;

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
