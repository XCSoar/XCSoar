/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Thread/SharedMutex.hpp"

#include <tchar.h>

struct NMEAInfo;
struct ComputerSettings;

class ProtectedTaskManager;

class Logger {
  LoggerImpl logger;
  mutable SharedMutex lock;

  void LogEvent(const NMEAInfo &gps_info, const char*);

public:
  void LogPoint(const NMEAInfo &gps_info);
  void LogStartEvent(const NMEAInfo &gps_info);
  void LogFinishEvent(const NMEAInfo &gps_info);

  gcc_pure
  bool IsLoggerActive() const;

  bool LoggerClearFreeSpace(unsigned current_year);
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
  void LoggerNote(const TCHAR *text);
  void ClearBuffer();
};

#endif
