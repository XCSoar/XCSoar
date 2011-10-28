/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_LOGGER_HPP
#define XCSOAR_LOGGER_HPP

#include "LoggerImpl.hpp"

#include <windows.h>
#include <tchar.h>
#include "Poco/RWLock.h"

struct NMEAInfo;
struct SETTINGS_COMPUTER;

class LoggerImpl;
class OrderedTask;
class ProtectedTaskManager;

class Logger {
private:
  LoggerImpl _logger;
  mutable Poco::RWLock lock;
  void LogEvent(const NMEAInfo &gps_info, const char*);

public:
  void LogPoint(const NMEAInfo &gps_info);
  void LogStartEvent(const NMEAInfo &gps_info);
  void LogFinishEvent(const NMEAInfo &gps_info);
  void LogTurnpointEvent(const NMEAInfo &gps_info);
  void LogOnTaskEvent(const NMEAInfo &gps_info);
  void LogPilotEvent(const NMEAInfo &gps_info);

  /**
   * Checks whether a Task is declared to the Logger.
   * If so, asks whether to invalidate the declaration.
   * @return True if a Task is NOT declared to the Logger, False otherwise
   */
  bool CheckDeclaration(void);

  gcc_pure
  bool IsTaskDeclared() const;

  gcc_pure
  bool IsLoggerActive() const;

  bool LoggerClearFreeSpace(const NMEAInfo &gps_info);
  void GUIStartLogger(const NMEAInfo& gps_info,
                      const SETTINGS_COMPUTER& settings,
                      const ProtectedTaskManager &protected_task_manager,
                      bool noAsk = false);
  void GUIToggleLogger(const NMEAInfo& gps_info,
                       const SETTINGS_COMPUTER& settings,
                       const ProtectedTaskManager &protected_task_manager,
                       bool noAsk = false);
  void GUIStopLogger(const NMEAInfo &gps_info,
                     bool noAsk = false);
  void LoggerDeviceDeclare(const OrderedTask& task);
  void LoggerNote(const TCHAR *text);
  void ClearBuffer();
};

#endif
