/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Computer/Settings.hpp"

void
Logger::LogPoint(const NMEAInfo &gps_info)
{
  // don't hold up the calculation thread if it's locked
  // by another process (most likely the logger gui message)

  const std::lock_guard protect{lock};
  logger.LogPoint(gps_info);
}

void
Logger::LogEvent(const NMEAInfo &gps_info, const char* event)
{
  const std::lock_guard protect{lock};
  logger.LogEvent(gps_info, event);
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

void
Logger::LogPilotEvent(const NMEAInfo &gps_info)
{
  LogEvent(gps_info, "PEV");
}

bool
Logger::IsLoggerActive() const noexcept
{
  const std::lock_guard protect{lock};
  return logger.IsActive();
}

void
Logger::GUIStartLogger(const NMEAInfo& gps_info,
                    const ComputerSettings& settings,
                       const ProtectedTaskManager *protected_task_manager,
                    bool noAsk)
{
  if (IsLoggerActive() || gps_info.gps.replay)
    return;

  auto task = protected_task_manager != nullptr
    ? protected_task_manager->TaskClone()
    : nullptr;
  const Declaration decl(settings.logger, settings.plane, task.get());

  if (task) {
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

  const std::lock_guard protect{lock};
  logger.StartLogger(gps_info, settings.logger, _T(""), decl);
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
    const std::lock_guard protect{lock};
    logger.StopLogger(gps_info);
  }
}

void
Logger::LoggerNote(const TCHAR *text)
{
  const std::lock_guard protect{lock};
  logger.LoggerNote(text);
}

void
Logger::ClearBuffer() noexcept
{
  const std::lock_guard protect{lock};
  logger.ClearBuffer();
}
