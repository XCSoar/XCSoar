/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "ProcessTimer.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Protection.hpp"
#include "Input/InputQueue.hpp"
#include "Input/InputEvents.hpp"
#include "Device/device.hpp"
#include "Device/All.hpp"
#include "Screen/Blank.hpp"
#include "UtilsSystem.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Time/PeriodClock.hpp"
#include "MainWindow.hpp"
#include "Asset.hpp"
#include "Simulator.hpp"
#include "Replay/Replay.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "BallastDumpManager.hpp"
#include "Operation/Operation.hpp"
#include "Tracking/TrackingGlue.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Event/Idle.hpp"

#ifdef _WIN32_WCE
static void
HeapCompact()
{
  static int iheapcompact = 0;
  // called 2 times per second, compact heap every minute.
  iheapcompact++;
  if (iheapcompact == 120) {
    MyCompactHeaps();
    iheapcompact = 0;
  }
}
#endif

static void
MessageProcessTimer()
{
  // don't display messages if airspace warning dialog is active
  if (CommonInterface::main_window->popup.Render())
    // turn screen on if blanked and receive a new message
    ResetUserIdle();
}

/**
 * Sets the system time to GPS time if not yet done and
 * defined in settings
 */
static void
SystemClockTimer()
{
#ifdef WIN32
  const NMEAInfo &basic = CommonInterface::Basic();

  // Altair doesn't have a battery-backed up realtime clock,
  // so as soon as we get a fix for the first time, set the
  // system clock to the GPS time.
  static bool sysTimeInitialised = false;

  if (basic.alive &&
      CommonInterface::GetComputerSettings().set_system_time_from_gps
      && basic.gps.real
      /* assume that we only have a valid date and time when we have a
         full GPS fix */
      && basic.location_available
      && !sysTimeInitialised) {
    SYSTEMTIME sysTime;
    ::GetSystemTime(&sysTime);

    sysTime.wYear = (unsigned short)basic.date_time_utc.year;
    sysTime.wMonth = (unsigned short)basic.date_time_utc.month;
    sysTime.wDay = (unsigned short)basic.date_time_utc.day;
    sysTime.wHour = (unsigned short)basic.date_time_utc.hour;
    sysTime.wMinute = (unsigned short)basic.date_time_utc.minute;
    sysTime.wSecond = (unsigned short)basic.date_time_utc.second;
    sysTime.wMilliseconds = 0;
    ::SetSystemTime(&sysTime);

#if defined(_WIN32_WCE) && defined(GNAV)
    TIME_ZONE_INFORMATION tzi;
    tzi.Bias = - CommonInterface::GetComputerSettings().utc_offset / 60;
    _tcscpy(tzi.StandardName,TEXT("Altair"));
    tzi.StandardDate.wMonth= 0; // disable daylight savings
    tzi.StandardBias = 0;
    _tcscpy(tzi.DaylightName,TEXT("Altair"));
    tzi.DaylightDate.wMonth= 0; // disable daylight savings
    tzi.DaylightBias = 0;

    SetTimeZoneInformation(&tzi);
#endif
    sysTimeInitialised =true;
  } else if (!basic.alive)
    /* set system clock again after a device reconnect; the new device
       may have a better GPS time */
    sysTimeInitialised = false;
#else
  // XXX
#endif
}

static void
SystemProcessTimer()
{
#ifdef _WIN32_WCE
  HeapCompact();
#endif

  SystemClockTimer();

  CheckDisplayTimeOut(false);
}

static void
BlackboardProcessTimer()
{
  device_blackboard->ExpireWallClock();
  XCSoarInterface::ExchangeBlackboard();
}

static void
BallastDumpProcessTimer()
{
  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();

  GlidePolar &glide_polar = settings_computer.polar.glide_polar_task;

  static BallastDumpManager ballast_manager;

  // Start/Stop the BallastDumpManager
  ballast_manager.SetEnabled(settings_computer.polar.ballast_timer_active);

  // If the BallastDumpManager is not enabled we must not call Update()
  if (!ballast_manager.IsEnabled())
    return;

  if (!ballast_manager.Update(glide_polar, settings_computer.plane.dump_time))
    // Plane is dry now -> disable ballast_timer
    settings_computer.polar.ballast_timer_active = false;

  if (protected_task_manager != NULL)
    protected_task_manager->SetGlidePolar(glide_polar);
}

static void
ManualWindProcessTimer()
{
  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  /* as soon as another wind setting is used, clear the manual wind */
  if (calculated.wind_available.Modified(settings_computer.wind.manual_wind_available))
    settings_computer.wind.manual_wind_available.Clear();
}

static void
SettingsProcessTimer()
{
  BallastDumpProcessTimer();
  ManualWindProcessTimer();
}

static void
CommonProcessTimer()
{
  BlackboardProcessTimer();

  SettingsProcessTimer();

  InfoBoxManager::ProcessTimer();
  InputEvents::ProcessTimer();

  MessageProcessTimer();
  SystemProcessTimer();
}

static void
ConnectionProcessTimer()
{
  static bool connected_last = false;
  static bool location_last = false;
  static bool wait_connect = false;

  const NMEAInfo &basic = CommonInterface::Basic();

  const bool connected_now = basic.alive,
    location_now = basic.location_available;
  if (connected_now) {
    if (location_now) {
      wait_connect = false;
    } else if (!connected_last || location_last) {
      // waiting for lock first time
      InputEvents::processGlideComputer(GCE_GPS_FIX_WAIT);
    }
  } else if (!connected_last) {
    if (!wait_connect) {
      // gps is waiting for connection first time
      wait_connect = true;
      InputEvents::processGlideComputer(GCE_GPS_CONNECTION_WAIT);
    }
  }

  connected_last = connected_now;
  location_last = location_now;

  /* this OperationEnvironment instance must be persistent, because
     DeviceDescriptor::Open() is asynchronous */
  static QuietOperationEnvironment env;
  AllDevicesAutoReopen(env);
}

void
ProcessTimer()
{
  CommonProcessTimer();

  if (!is_simulator()) {
    // now check GPS status
    devTick();

    // also service replay logger
    if (replay && replay->IsActive()) {
      if (CommonInterface::MovementDetected())
        replay->Stop();
    }

    ConnectionProcessTimer();
  } else {
    static PeriodClock m_clock;

    if (replay && replay->IsActive()) {
      m_clock.Update();
    } else if (m_clock.Elapsed() >= 1000) {
      m_clock.Update();
      device_blackboard->ProcessSimulation();
    } else if (!m_clock.IsDefined())
      m_clock.Update();
  }

#ifdef HAVE_TRACKING
  if (tracking != NULL && CommonInterface::Basic().gps.real) {
    tracking->SetSettings(CommonInterface::GetComputerSettings().tracking);
    tracking->OnTimer(CommonInterface::Basic(), CommonInterface::Calculated());
  }
#endif
}
