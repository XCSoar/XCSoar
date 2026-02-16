// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
      char TaskMessage[1024];
      strcpy(TaskMessage, _T("Start Logger With Declaration\r\n"));
      
      if (decl.Size()) {
        for (unsigned i = 0; i< decl.Size(); ++i) {
          strcat(TaskMessage, decl.GetName(i));
          strcat(TaskMessage, _T("\r\n"));
        }
      } else {
        strcat(TaskMessage, _T("None"));
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
Logger::LoggerNote(const char *text)
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
