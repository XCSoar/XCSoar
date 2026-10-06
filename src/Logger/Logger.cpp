// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Logger/Logger.hpp"
#include "Logger/GPSDeviceName.hpp"
#include "Device/Declaration.hpp"
#include "NMEA/Info.hpp"
#include "Language/Language.hpp"
#include "Dialogs/Message.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Computer/Settings.hpp"
#include "Interface.hpp"
#include "Simulator.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Device/Features.hpp"

#include <array>
#include <optional>

/**
 * The device that supplies the GPS fix right now, for the HFGPS
 * header and the L record of a new source (#3180).
 */
static std::optional<unsigned>
GetGPSDeviceIndex() noexcept
{
  if (backend_components == nullptr ||
      backend_components->device_blackboard == nullptr)
    return std::nullopt;

  auto &blackboard = *backend_components->device_blackboard;
  const std::lock_guard lock{blackboard.mutex};

  std::array<const NMEAInfo *, NUMDEV> devices;
  for (unsigned i = 0; i < NUMDEV; ++i)
    devices[i] = &blackboard.RealState(i);

  return FindGPSDevice(devices);
}

void
Logger::LogPoint(const NMEAInfo &gps_info)
{
  const auto source = GetGPSDeviceIndex();

  // don't hold up the calculation thread if it's locked
  // by another process (most likely the logger gui message)

  const std::lock_guard protect{lock};
  const bool written = logger.LogPoint(gps_info);

  /* HFGPS cannot change after the header; note a new source in an L
     record after the first fix it supplied */
  if (written && source && *source != gps_device) {
    gps_device = *source;
    logger.LogGPSSource(gps_info, char('A' + gps_device),
                        gps_device_names[gps_device]);
  }
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

AllocatedPath
Logger::GetActivePath() const noexcept
{
  const std::lock_guard protect{lock};
  return AllocatedPath(logger.GetPath());
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
      strcpy(TaskMessage, "Start Logger With Declaration\r\n");
      
      if (decl.Size()) {
        for (unsigned i = 0; i< decl.Size(); ++i) {
          strcat(TaskMessage, decl.GetName(i));
          strcat(TaskMessage, "\r\n");
        }
      } else {
        strcat(TaskMessage, "None");
      }
      
      if (ShowMessageBox(TaskMessage, _("Start Logger"),
                      MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;
    }
  }

  const auto source = GetGPSDeviceIndex();
  const auto &devices = CommonInterface::GetSystemSettings().devices;

  const std::lock_guard protect{lock};
  for (unsigned i = 0; i < NUMDEV; ++i)
    gps_device_names[i] = GetGPSDeviceName(devices[i], is_simulator());

  /* device A if none supplies a fix yet */
  gps_device = source.value_or(0);

  logger.StartLogger(gps_info, settings.logger, "", decl,
                     gps_device_names[gps_device]);
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
