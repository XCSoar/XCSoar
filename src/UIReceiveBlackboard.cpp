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

  /* Most InfoBoxes are refreshed by calculated updates.  Merge-rate STF
     (MoreData::V_stf) also dirties boxes when availability or value changes.
     Compare as bool so Validity timestamps alone do not thrash redraws. */
  static bool last_stf_available = false;
  static double last_stf = 0;
  const auto &basic = CommonInterface::Basic();
  const bool stf_available = (bool)basic.V_stf_available;
  const bool stf_changed = stf_available != last_stf_available ||
    (stf_available && basic.V_stf != last_stf);
  last_stf_available = stf_available;
  if (stf_available)
    last_stf = basic.V_stf;

  if (modified || !basic.location_available || stf_changed) {
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
  ActionInterface::ScheduleSendUIState();

  if (backend_components->devices)
    backend_components->devices->NotifyCalculatedUpdate(CommonInterface::Basic(),
                                                        CommonInterface::Calculated());

  if (backend_components->protected_task_manager) {
    const ProtectedTaskManager::Lease lease{*backend_components->protected_task_manager};
    task_event_observer.Check(lease);
  }
}
