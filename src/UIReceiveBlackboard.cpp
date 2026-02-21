// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UIReceiveBlackboard.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "ApplyVegaSwitches.hpp"
#include "ApplyExternalSettings.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Device/MultipleDevices.hpp"
#include "Input/TaskEventObserver.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "FlarmProgressOverlay.hpp"
#include "util/StaticString.hxx"

#if defined(__linux__) && defined(USE_POLL_EVENT) && !defined(KOBO)
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/TimeDate.hxx"
#include "LogFile.hpp"

#include <unistd.h> // for geteuid()
#endif

static TaskEventObserver task_event_observer;

void
UIReceiveSensorData(OperationEnvironment &env)
{
  XCSoarInterface::ReceiveGPS();

  ApplyVegaSwitches();

#if defined(__linux__) && defined(USE_POLL_EVENT) && !defined(KOBO)
  static bool clock_set = false;
  if (const auto &basic = CommonInterface::Basic();
      !clock_set &&
      basic.gps.real &&
      basic.time_available &&
      CommonInterface::Basic().date_time_utc.IsDatePlausible() &&
      geteuid() == 0) {
    /* copy the GPS clock to the system clock if NTP synchronizarion
       hasn't been done yet (maybe because we are airborne and have no
       internet connection); this feature is designed for OpenVario
       where geteuid()==0 */

    clock_set = true;

    try {
      auto connection = ODBus::Connection::GetSystem();
      if (!TimeDate::IsNTPSynchronized(connection))
        TimeDate::SetTime(connection, CommonInterface::Basic().date_time_utc.ToTimePoint());
      LogFormat("System clock set from GPS");
    } catch (...) {
      LogError(std::current_exception(), "Failed to set the system clock from GPS");
    }
  }
#endif

  bool modified = ApplyExternalSettings(env);

  /*
   * Update the infoboxes if no location is available
   *
   * (if the location is available the CalculationThread will send the
   * Command::CALCULATED_UPDATE message which will update them)
   */
  if (modified || !CommonInterface::Basic().location_available) {
    InfoBoxManager::SetDirty();
    InfoBoxManager::ProcessTimer();
  }

  {
    static bool flarm_progress_visible = false;
    const auto &progress = CommonInterface::Basic().flarm.progress;

    if (progress.available) {
      StaticString<128> msg;
      if (!progress.info.empty())
        msg.Format("FLARM %s: %s (%u%%)",
                   progress.operation.c_str(),
                   progress.info.c_str(),
                   progress.progress);
      else
        msg.Format("FLARM %s (%u%%)",
                   progress.operation.c_str(),
                   progress.progress);

      FlarmProgressOverlay::Show(msg.c_str(), progress.progress);
      flarm_progress_visible = true;
    } else if (flarm_progress_visible) {
      FlarmProgressOverlay::Close();
      flarm_progress_visible = false;
    }
  }
}

void
UIReceiveCalculatedData()
{
  XCSoarInterface::ReceiveCalculated();

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState();

  if (backend_components->devices)
    backend_components->devices->NotifyCalculatedUpdate(CommonInterface::Basic(),
                                                        CommonInterface::Calculated());

  if (backend_components->protected_task_manager) {
    const ProtectedTaskManager::Lease lease{*backend_components->protected_task_manager};
    task_event_observer.Check(lease);
  }
}
