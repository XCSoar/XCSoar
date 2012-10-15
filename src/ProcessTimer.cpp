/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Protection.hpp"
#include "Input/InputQueue.hpp"
#include "Input/InputEvents.hpp"
#include "Device/device.hpp"
#include "Device/All.hpp"
#include "Dialogs/AirspaceWarningDialog.hpp"
#include "Screen/Blank.hpp"
#include "UtilsSystem.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "PeriodClock.hpp"
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
  if (!dlgAirspaceWarningVisible())
    if (CommonInterface::main_window->popup.Render())
      // turn screen on if blanked and receive a new message
      ResetDisplayTimeOut();
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

/**
 * Collect QNH, MacCready, Ballast and Bugs updates from external devices
 */
static void
BallastProcessTimer()
{
  static Validity last_fraction, last_overload;
  const ExternalSettings &settings = CommonInterface::Basic().settings;

  if (settings.ballast_fraction_available.Modified(last_fraction))
    ActionInterface::SetBallast(settings.ballast_fraction, false);

  last_fraction = settings.ballast_fraction_available;

  if (settings.ballast_overload_available.Modified(last_overload)) {

    const Plane &plane = CommonInterface::GetComputerSettings().plane;

    if (plane.max_ballast > fixed_zero) {
      fixed fraction = ((settings.ballast_overload - fixed_one) *
                        plane.dry_mass) / plane.max_ballast;
      ActionInterface::SetBallast(fraction, false);
    }
  }

  last_overload = settings.ballast_overload_available;
}

static void
BugsProcessTimer()
{
  static Validity last;
  const ExternalSettings &settings = CommonInterface::Basic().settings;

  if (settings.bugs_available.Modified(last))
    ActionInterface::SetBugs(settings.bugs, false);

  last = settings.bugs_available;
}

static void
QNHProcessTimer()
{
  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (basic.settings.qnh_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = basic.settings.qnh;
    settings_computer.pressure_available = basic.settings.qnh_available;
  }

  if (calculated.pressure_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = calculated.pressure;
    settings_computer.pressure_available = calculated.pressure_available;

    MessageOperationEnvironment env;
    AllDevicesPutQNH(settings_computer.pressure, env);
  }
}

static void
MacCreadyProcessTimer()
{
  static ExternalSettings last_external_settings;
  static Validity last_auto_mac_cready;

  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (last_auto_mac_cready.Modified(calculated.auto_mac_cready_available))
    /* time warp, reset */
    last_auto_mac_cready.Clear();

  if (basic.settings.mac_cready_available.Modified(last_external_settings.mac_cready_available)) {
    ActionInterface::SetMacCready(basic.settings.mac_cready, false);
  } else if (calculated.auto_mac_cready_available.Modified(last_auto_mac_cready)) {
    last_auto_mac_cready = calculated.auto_mac_cready_available;
    ActionInterface::SetMacCready(calculated.auto_mac_cready);
  }

  last_external_settings = basic.settings;
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
  BallastProcessTimer();
  BugsProcessTimer();
  QNHProcessTimer();
  MacCreadyProcessTimer();
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
    devTick(CommonInterface::Calculated());

    // also service replay logger
    if (replay && replay->Update()) {
      if (CommonInterface::MovementDetected())
        replay->Stop();
    }

    ConnectionProcessTimer();
  } else {
    static PeriodClock m_clock;
    if (m_clock.Elapsed() < 0)
      m_clock.Update();

    if (replay && replay->Update()) {
      m_clock.Update();
    } else if (m_clock.Elapsed() >= 1000) {
      m_clock.Update();
      device_blackboard->ProcessSimulation();
    }
  }

#ifdef HAVE_TRACKING
  if (tracking != NULL && CommonInterface::Basic().gps.real) {
    tracking->SetSettings(CommonInterface::GetComputerSettings().tracking);
    tracking->OnTimer(CommonInterface::Basic(), CommonInterface::Calculated());
  }
#endif
}
