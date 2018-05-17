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

#include "Logger/Logger.hpp"
#include "Device/Declaration.hpp"
#include "NMEA/Info.hpp"
#include "Language/Language.hpp"
#include "Dialogs/Message.hpp"
#include "LogFile.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Asset.hpp"
#include "Computer/Settings.hpp"
#include "IGCFileCleanup.hpp"

void
Logger::LogPoint(const NMEAInfo &gps_info)
{
  // don't hold up the calculation thread if it's locked
  // by another process (most likely the logger gui message)

  if (lock.try_lock()) {
    logger.LogPoint(gps_info);
    lock.unlock();
  }
}

void
Logger::LogEvent(const NMEAInfo &gps_info, const char* event)
{
  if (lock.try_lock()) {
    logger.LogEvent(gps_info, event);
    lock.unlock();
  }
}

void
Logger::LogStartEvent(const NMEAInfo &gps_info)
{
  LogEvent(gps_info, "STA");
}

void
Logger::LogFinishEvent(const NMEAInfo &gps_info)
{
  LogEvent(gps_info, "FIN");
}

bool
Logger::IsLoggerActive() const
{
  const ScopeSharedLock protect(lock);
  return logger.IsActive();
}

bool
Logger::LoggerClearFreeSpace(unsigned current_year)
{
  return IGCFileCleanup(current_year);
}

void
Logger::GUIStartLogger(const NMEAInfo& gps_info,
                    const ComputerSettings& settings,
                       const ProtectedTaskManager *protected_task_manager,
                    bool noAsk)
{
  if (IsLoggerActive() || gps_info.gps.replay)
    return;

  OrderedTask* task = protected_task_manager != nullptr
    ? protected_task_manager->TaskClone()
    : nullptr;
  const Declaration decl(settings.logger, settings.plane, task);

  if (task) {
    delete task;

    if (!noAsk) {
      TCHAR TaskMessage[1024];
      _tcscpy(TaskMessage, _T("Start Logger With Declaration\r\n"));
      
      if (decl.Size()) {
        for (unsigned i = 0; i< decl.Size(); ++i) {
          _tcscat(TaskMessage, decl.GetName(i));
          _tcscat(TaskMessage, _T("\r\n"));
        }
      } else {
        _tcscat(TaskMessage, _T("None"));
      }
      
      if (ShowMessageBox(TaskMessage, _("Start Logger"),
                      MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;
    }
  }

  if (!LoggerClearFreeSpace(gps_info.date_time_utc.year)) {
    ShowMessageBox(_("Logger inactive, insufficient storage!"),
                _("Logger Error"), MB_OK| MB_ICONERROR);
    LogFormat("Logger not started: Insufficient Storage");
    return;
  }

  const ScopeExclusiveLock protect(lock);
  logger.StartLogger(gps_info, settings.logger, asset_number, decl);
}

void
Logger::GUIToggleLogger(const NMEAInfo& gps_info,
                     const ComputerSettings& settings,
                      const ProtectedTaskManager *protected_task_manager,
                     bool noAsk)
{
  if (IsLoggerActive())
    GUIStopLogger(gps_info, noAsk);
  else
    GUIStartLogger(gps_info, settings, protected_task_manager, noAsk);
}

void
Logger::GUIStopLogger(const NMEAInfo &gps_info,
                   bool noAsk)
{
  if (!IsLoggerActive())
    return;

  if (noAsk || (ShowMessageBox(_("Stop Logger"), _("Stop Logger"),
                            MB_YESNO | MB_ICONQUESTION) == IDYES)) {
    const ScopeExclusiveLock protect(lock);
    logger.StopLogger(gps_info);
  }
}

void
Logger::LoggerNote(const TCHAR *text)
{
  const ScopeExclusiveLock protect(lock);
  logger.LoggerNote(text);
}

void
Logger::ClearBuffer()
{
  const ScopeExclusiveLock protect(lock);
  logger.ClearBuffer();
}
