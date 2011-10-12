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

#include "ProcessTimer.hpp"
#include "Interface.hpp"
#include "Protection.hpp"
#include "InputEvents.hpp"
#include "Device/device.hpp"
#include "Device/All.hpp"
#include "Dialogs/AirspaceWarningDialog.hpp"
#include "Screen/Blank.hpp"
#include "UtilsSystem.hpp"
#include "DeviceBlackboard.hpp"
#include "Components.hpp"
#include "PeriodClock.hpp"
#include "MainWindow.hpp"
#include "Asset.hpp"
#include "Simulator.hpp"
#include "Replay/Replay.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "GPSClock.hpp"
#include "Operation.hpp"
#include "Tracking/TrackingGlue.hpp"

#ifdef _WIN32_WCE
void
ProcessTimer::HeapCompact()
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

void
ProcessTimer::MessageProcessTimer()
{
  // don't display messages if airspace warning dialog is active
  if (!dlgAirspaceWarningVisible())
    if (CommonInterface::main_window.popup.Render())
      // turn screen on if blanked and receive a new message
      ResetDisplayTimeOut();
}

void
ProcessTimer::AirspaceProcessTimer()
{
  static Validity previous;

  const AirspaceWarningsInfo &warnings =
    CommonInterface::Calculated().airspace_warnings;

  if (previous.Modified(warnings.latest))
    /* time warp, reset */
    previous.Clear();

  if (warnings.latest.Modified(previous)) {
    previous = warnings.latest;
    CommonInterface::main_window.SendAirspaceWarning();
  }
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

  if (basic.connected &&
      CommonInterface::SettingsComputer().SetSystemTimeFromGPS
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
    tzi.Bias = - CommonInterface::SettingsComputer().UTCOffset / 60;
    _tcscpy(tzi.StandardName,TEXT("Altair"));
    tzi.StandardDate.wMonth= 0; // disable daylight savings
    tzi.StandardBias = 0;
    _tcscpy(tzi.DaylightName,TEXT("Altair"));
    tzi.DaylightDate.wMonth= 0; // disable daylight savings
    tzi.DaylightBias = 0;

    SetTimeZoneInformation(&tzi);
#endif
    sysTimeInitialised =true;
  } else if (!basic.connected)
    /* set system clock again after a device reconnect; the new device
       may have a better GPS time */
    sysTimeInitialised = false;
#else
  // XXX
#endif
}

void
ProcessTimer::SystemProcessTimer()
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
  device_blackboard.expire_wall_clock();
  XCSoarInterface::ExchangeBlackboard();
}

/**
 * Collect QNH updates from external devices and AutoQNH.
 */
static void
QNHProcessTimer()
{
  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (basic.settings.qnh_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = basic.settings.qnh;
    settings_computer.pressure_available = basic.settings.qnh_available;
  }

  if (calculated.pressure_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = calculated.pressure;
    settings_computer.pressure_available = calculated.pressure_available;

    AllDevicesPutQNH(settings_computer.pressure);
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
  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();

  if (!settings_computer.BallastTimerActive)
    return;

  // only update every 5 seconds to stop flooding the devices
  static GPSClock ballast_clock(fixed(5));

  const NMEAInfo &basic = CommonInterface::Basic();

  // We don't know how fast the water is flowing so don't pretend that we do
  if (settings_computer.plane.dump_time <= 0) {
    settings_computer.BallastTimerActive = false;
    return;
  }

  fixed dt = ballast_clock.delta_advance(basic.time);

  if (negative(dt))
    return;

  GlidePolar &glide_polar = settings_computer.glide_polar_task;
  fixed ballast = glide_polar.GetBallast();
  fixed percent_per_second = fixed_one / settings_computer.plane.dump_time;

  ballast -= dt * percent_per_second;
  if (negative(ballast)) {
    settings_computer.BallastTimerActive = false;
    ballast = fixed_zero;
  }

  glide_polar.SetBallast(ballast);

  if (protected_task_manager != NULL)
    protected_task_manager->SetGlidePolar(glide_polar);
}

static void
ManualWindProcessTimer()
{
  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  /* as soon as another wind setting is used, clear the manual wind */
  if (calculated.wind_available.Modified(settings_computer.ManualWindAvailable))
    settings_computer.ManualWindAvailable.Clear();
}

static void
SettingsProcessTimer()
{
  QNHProcessTimer();
  MacCreadyProcessTimer();
  BallastDumpProcessTimer();
  ManualWindProcessTimer();
}

void
ProcessTimer::CommonProcessTimer()
{
  BlackboardProcessTimer();

  ActionInterface::DisplayModes();

  SettingsProcessTimer();

  InfoBoxManager::ProcessTimer();
  InputEvents::ProcessTimer();

  AirspaceProcessTimer();
  MessageProcessTimer();
  SystemProcessTimer();
}

int
ProcessTimer::ConnectionProcessTimer(int itimeout)
{
  if (AllDevicesIsBusy())
    /* don't check for device timeouts during task declaration, as the
       device is busy with that and will not send NMEA updates */
    return itimeout;

  static bool connected_last = false;
  static bool location_last = false;
  static bool wait_connect = false;

  const NMEAInfo &basic = CommonInterface::Basic();

  bool connected_now = basic.connected;
  if (connected_now) {
    if (basic.location_available) {
      wait_connect = false;
      itimeout = 0;
    } else if (!connected_last || location_last) {
      // waiting for lock first time
      itimeout = 0;
      InputEvents::processGlideComputer(GCE_GPS_FIX_WAIT);
    }
  }

  if (!connected_now && !connected_last) {
    if (!wait_connect) {
      // gps is waiting for connection first time
      wait_connect = true;
      InputEvents::processGlideComputer(GCE_GPS_CONNECTION_WAIT);
    }
  }

  PopupOperationEnvironment env;
  AllDevicesAutoReopen(env);

  connected_last = connected_now;
  location_last = basic.location_available;
  return itimeout;
}

void
ProcessTimer::Process(void)
{
  CommonProcessTimer();

  if (!is_simulator()) {
    // now check GPS status
    devTick(CommonInterface::Calculated());

    static int itimeout = -1;
    itimeout++;

    // also service replay logger
    if (replay && replay->Update()) {
      if (CommonInterface::MovementDetected())
        replay->Stop();

      return;
    }

    if (itimeout % 10 == 0)
      // check connection status every 5 seconds
      itimeout = ConnectionProcessTimer(itimeout);
  } else {
    static PeriodClock m_clock;
    if (m_clock.elapsed() < 0)
      m_clock.update();

    if (replay && replay->Update()) {
      m_clock.update();
    } else if (m_clock.elapsed() >= 1000) {
      m_clock.update();
      device_blackboard.ProcessSimulation();
    }
  }

#ifdef HAVE_TRACKING
  if (tracking != NULL && CommonInterface::Basic().gps.real) {
    tracking->SetSettings(CommonInterface::SettingsComputer().tracking);
    tracking->OnTimer(CommonInterface::Basic());
  }
#endif
}
