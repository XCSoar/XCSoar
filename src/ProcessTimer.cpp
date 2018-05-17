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

#include "ProcessTimer.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Input/InputQueue.hpp"
#include "Input/InputEvents.hpp"
#include "Device/MultipleDevices.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Time/PeriodClock.hpp"
#include "MainWindow.hpp"
#include "PopupMessage.hpp"
#include "Simulator.hpp"
#include "Replay/Replay.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "BallastDumpManager.hpp"
#include "Operation/Operation.hpp"
#include "Tracking/TrackingGlue.hpp"
#include "Event/Idle.hpp"
#include "Dialogs/Tracking/CloudEnableDialog.hpp"

static void
MessageProcessTimer()
{
  // don't display messages if airspace warning dialog is active
  if (CommonInterface::main_window->popup != nullptr &&
      CommonInterface::main_window->popup->Render())
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

  // as soon as we get a fix for the first time, set the
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
  SystemClockTimer();
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

  if (protected_task_manager != nullptr)
    protected_task_manager->SetGlidePolar(glide_polar);
}

static void
ProcessAutoBugs()
{
  /**
   * Increase the bugs value every hour.
   */
  static constexpr double interval(3600);

  /**
   * Decrement the bugs setting by 1%.
   */
  static constexpr double decrement(0.01);

  /**
   * Don't go below this bugs setting.
   */
  static constexpr double min_bugs(0.7);

  /**
   * The time stamp (from FlyingState::flight_time) when we last
   * increased the bugs value automatically.
   */
  static double last_auto_bugs;

  const FlyingState &flight = CommonInterface::Calculated().flight;
  const PolarSettings &polar = CommonInterface::GetComputerSettings().polar;

  if (!flight.flying)
    /* reset when not flying */
    last_auto_bugs = 0;
  else if (!polar.auto_bugs)
    /* feature is disabled */
    last_auto_bugs = flight.flight_time;
  else if (flight.flight_time >= last_auto_bugs + interval &&
           polar.bugs > min_bugs) {
    last_auto_bugs = flight.flight_time;
    ActionInterface::SetBugs(std::max(polar.bugs - decrement, min_bugs));
  }
}

static void
SettingsProcessTimer()
{
  CloudEnableDialog();
  BallastDumpProcessTimer();
  ProcessAutoBugs();
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
  if (devices == nullptr)
    return;

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
  devices->AutoReopen(env);
}

void
ProcessTimer()
{
  CommonProcessTimer();

  if (!is_simulator()) {
    // now check GPS status
    if (devices != nullptr)
      devices->Tick();

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
  if (tracking != nullptr) {
    tracking->SetSettings(CommonInterface::GetComputerSettings().tracking);
    tracking->OnTimer(CommonInterface::Basic(), CommonInterface::Calculated());
  }
#endif
}
