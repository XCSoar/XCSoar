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

#include "Logger/Logger.hpp"
#include "Logger/ExternalLogger.hpp"
#include "Device/Declaration.hpp"
#include "NMEA/Info.hpp"
#include "Language/Language.hpp"
#include "Dialogs/Message.hpp"
#include "LogFile.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Asset.hpp"
#include "Profile/DeclarationConfig.hpp"
#include "SettingsComputerBlackboard.hpp"

void
Logger::LogPoint(const NMEAInfo &gps_info)
{
  // don't hold up the calculation thread if it's locked
  // by another process (most likely the logger gui message)

  if (lock.tryWriteLock()) {
    _logger.LogPoint(gps_info);
    lock.unlock();
  }
}

void
Logger::LogEvent(const NMEAInfo &gps_info, const char* event)
{
  if (lock.tryWriteLock()) {
    _logger.LogEvent(gps_info, event);
    lock.unlock();
  }

}

void Logger::LogStartEvent(const NMEAInfo &gps_info) {
  LogEvent(gps_info, "STA");
}

void Logger::LogFinishEvent(const NMEAInfo &gps_info) {
  LogEvent(gps_info, "FIN");
}

void Logger::LogPilotEvent(const NMEAInfo &gps_info) {
  LogEvent(gps_info, "PEV");
}

void Logger::LogTurnpointEvent(const NMEAInfo &gps_info) {
  LogEvent(gps_info, "TPC");
}


/**
 * Checks whether a Task is declared to the Logger.
 * If so, asks whether to invalidate the declaration.
 * @return True if a Task is NOT declared to the Logger, False otherwise
 */
bool
Logger::CheckDeclaration(void)
{
  return ExternalLogger::CheckDeclaration();
}

bool
Logger::isTaskDeclared() const
{
  return ExternalLogger::IsDeclared();
}

bool
Logger::isLoggerActive() const
{
  Poco::ScopedRWLock protect(lock, false);
  return _logger.isLoggerActive();
}

bool
Logger::LoggerClearFreeSpace(const NMEAInfo &gps_info)
{
  Poco::ScopedRWLock protect(lock, true);
  return _logger.LoggerClearFreeSpace(gps_info);
}


void
Logger::guiStartLogger(const NMEAInfo& gps_info,
                    const SETTINGS_COMPUTER& settings,
                       const ProtectedTaskManager &protected_task_manager,
                    bool noAsk)
{
  if (isLoggerActive() || gps_info.gps.replay)
    return;

  OrderedTask* task = protected_task_manager.task_clone();
  Declaration decl(task);
  Profile::GetDeclarationConfig(decl, settings.plane);

  if (task) {
    delete task;

    if (!noAsk) {
      TCHAR TaskMessage[1024];
      _tcscpy(TaskMessage, _T("Start Logger With Declaration\r\n"));
      
      if (decl.size()) {
        for (unsigned i = 0; i< decl.size(); ++i) {
          _tcscat(TaskMessage, decl.get_name(i));
          _tcscat(TaskMessage, _T("\r\n"));
        }
      } else {
        _tcscat(TaskMessage, _T("None"));
      }
      
      if (MessageBoxX(TaskMessage, _("Start Logger"),
                      MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;
    }
  }

  if (!LoggerClearFreeSpace(gps_info)) {
    MessageBoxX(_("Logger inactive, insufficient storage!"),
                _("Logger Error"), MB_OK| MB_ICONERROR);
    LogStartUp(_T("Logger not started: Insufficient Storage"));
    return;
  }

  Poco::ScopedRWLock protect(lock, true);
  _logger.StartLogger(gps_info, settings, strAssetNumber, decl);
}

void
Logger::guiToggleLogger(const NMEAInfo& gps_info,
                     const SETTINGS_COMPUTER& settings,
                      const ProtectedTaskManager &protected_task_manager,
                     bool noAsk)
{
  if (isLoggerActive()) {
    guiStopLogger(gps_info, noAsk);
  } else {
    guiStartLogger(gps_info, settings, protected_task_manager, noAsk);
  }
}

void
Logger::guiStopLogger(const NMEAInfo &gps_info,
                   bool noAsk)
{
  if (!isLoggerActive())
    return;

  if (noAsk || (MessageBoxX(_("Stop Logger"),
                            _("Stop Logger"),
                            MB_YESNO | MB_ICONQUESTION) == IDYES)) {
    Poco::ScopedRWLock protect(lock, true);
    _logger.StopLogger(gps_info);
  }
}

void
Logger::LoggerDeviceDeclare(const OrderedTask& task)
{
  return ExternalLogger::Declare(task);
}

void
Logger::LoggerNote(const TCHAR *text)
{
  Poco::ScopedRWLock protect(lock, true);
  return _logger.LoggerNote(text);
}

void
Logger::clearBuffer()
{
  Poco::ScopedRWLock protect(lock, true);
  return _logger.clearBuffer();
}
