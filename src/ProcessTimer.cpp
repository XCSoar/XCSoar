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
#include "Protection.hpp"
#include "Interface.hpp"
#include "InputEvents.hpp"
#include "Device/device.hpp"
#include "Device/All.hpp"
#include "Dialogs/Dialogs.h"
#include "Screen/Blank.hpp"
#include "UtilsSystem.hpp"
#include "DeviceBlackboard.hpp"
#include "Components.hpp"
#include "PeriodClock.hpp"
#include "MainWindow.hpp"
#include "Asset.hpp"
#include "Simulator.hpp"
#include "Replay/Replay.hpp"
#include "Audio/Sound.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"

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

void
ProcessTimer::SystemProcessTimer()
{
  HeapCompact();
}

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
  if (airspaceWarningEvent.test()) {
    airspaceWarningEvent.reset();
    ResetDisplayTimeOut();
#ifndef GNAV
    PlayResource(_T("IDR_WAV_BEEPBWEEP"));
#endif
    dlgAirspaceWarningsShowModal(XCSoarInterface::main_window, true);
  }
}

/**
 * Sets the system time to GPS time if not yet done and
 * defined in settings
 */
static void
SystemClockTimer()
{
  if (is_simulator())
    return;

#ifdef WIN32
  const NMEA_INFO &basic = CommonInterface::Basic();

  // Altair doesn't have a battery-backed up realtime clock,
  // so as soon as we get a fix for the first time, set the
  // system clock to the GPS time.
  static bool sysTimeInitialised = false;

  if (basic.Connected && CommonInterface::SettingsMap().SetSystemTimeFromGPS
      && !sysTimeInitialised) {
    SYSTEMTIME sysTime;
    ::GetSystemTime(&sysTime);

    sysTime.wYear = (unsigned short)basic.DateTime.year;
    sysTime.wMonth = (unsigned short)basic.DateTime.month;
    sysTime.wDay = (unsigned short)basic.DateTime.day;
    sysTime.wHour = (unsigned short)basic.DateTime.hour;
    sysTime.wMinute = (unsigned short)basic.DateTime.minute;
    sysTime.wSecond = (unsigned short)basic.DateTime.second;
    sysTime.wMilliseconds = 0;
    sysTimeInitialised = (::SetSystemTime(&sysTime)==true);

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
  }
#else
  // XXX
#endif
}

void
ProcessTimer::CommonProcessTimer()
{
  SystemClockTimer();

  CheckDisplayTimeOut(false);

  ActionInterface::DisplayModes();
  XCSoarInterface::ExchangeBlackboard();

  InfoBoxManager::ProcessTimer();
  InputEvents::ProcessTimer();

  AirspaceProcessTimer();
  MessageProcessTimer();

  SystemClockTimer();
}

int
ProcessTimer::ConnectionProcessTimer(int itimeout)
{
  if (AllDevicesIsDeclaring())
    /* don't check for device timeouts during task declaration, as the
       device is busy with that and will not send NMEA updates */
    return itimeout;

  devConnectionMonitor();

  static bool connected_last = false;
  static bool wait_connect = false;
  static bool wait_lock = false;

  const NMEA_INFO &basic = CommonInterface::Basic();

  bool connected_now = basic.Connected;
  if (connected_now && !basic.LocationAvailable) {
    if (!wait_lock) {
      // waiting for lock first time
      wait_lock = true;
      itimeout = 0;
      InputEvents::processGlideComputer(GCE_GPS_FIX_WAIT);
#ifndef DISABLEAUDIO
      MessageBeep(MB_ICONEXCLAMATION);
#endif
    }
  } else if (connected_now) {
    // !navwarning
    wait_connect = false;
    wait_lock = false;
    itimeout = 0;
  } else {
    // not connected
    wait_lock = false;
  }

  if (!connected_now && !connected_last) {
    AllDevicesLinkTimeout();

    if (!wait_connect) {
      // gps is waiting for connection first time
      wait_connect = true;
      InputEvents::processGlideComputer(GCE_GPS_CONNECTION_WAIT);
#ifndef DISABLEAUDIO
      MessageBeep(MB_ICONEXCLAMATION);
#endif
    } else if (itimeout % 60 == 0) {
      itimeout = 0;
      // we've been waiting for connection a long time
      // no activity for 30 seconds, so assume PDA has been
      // switched off and on again
      //
      if (!is_altair()) {
        InputEvents::processGlideComputer(GCE_COMMPORT_RESTART);
        devRestart();
      }
    }
  }

  connected_last = connected_now;
  return itimeout;
}

void
ProcessTimer::Process(void)
{
  if (device_blackboard.expire_wall_clock())
    /* trigger a redraw when the connection was just lost, to show the
       new state; when no GPS is connected, no other entity triggers
       the redraw, so we have to do it */
    CommonInterface::main_window.full_redraw();

  CommonProcessTimer();

  if (!is_simulator()) {
    // now check GPS status
    devTick();

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
}

/*
#include "Waypoint/Waypoints.hpp"
  // if transition to flight, ensure there is a temporary waypoint available
  // at this location for general use.
  if (Calculated().Flight.Flying && !LastCalculated().Flight.Flying && (Basic().Time> LastBasic().Time)) {
    const AIRCRAFT_STATE current_as = ToAircraftState(basic, Calculated());
    const GeoPoint& location = current_as.get_location();
    const Waypoint *from_database = way_points.lookup_location(location, fixed(3000));
    // threshold must be large enough for runway takeoff length!

      if (from_database == NULL) {
        // need to create one
        Waypoint wp(location);
        wp.Name = _T("(takeoff)");
        if (Calculated().TerrainValid) {
          wp.Altitude = Calculated().TerrainAlt;
        } else {
          wp.Altitude = Basic().NavAltitude;
        }
        way_points.append(wp);
        way_points.optimise();
      }
    }
*/
