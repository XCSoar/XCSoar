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
    dlgAirspaceWarningShowDlg();
  }
}

void
ProcessTimer::CommonProcessTimer()
{
  CheckDisplayTimeOut(false);

  ActionInterface::DisplayModes();
  XCSoarInterface::ExchangeBlackboard();

  InfoBoxManager::ProcessTimer();
  InputEvents::ProcessTimer();

  AirspaceProcessTimer();
  MessageProcessTimer();
  SystemProcessTimer();
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

  const GPS_STATE &gps = CommonInterface::Basic().gps;

  if (!gps.Connected)
    // if gps is not connected, set navwarning to true so
    // calculations flight timers don't get updated
    device_blackboard.SetNAVWarning(true);

  bool connected_now = device_blackboard.LowerConnection();
  if (connected_now && gps.NAVWarning) {
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
  CommonProcessTimer();

  if (!is_simulator()) {
    // now check GPS status
    devTick();

    static int itimeout = -1;
    itimeout++;

    // also service replay logger
    if (replay.Update()) {
      if (CommonInterface::Basic().gps.MovementDetected &&
          !replay.NmeaReplayEnabled())
        replay.Stop();

      device_blackboard.RaiseConnection();
      device_blackboard.SetNAVWarning(false);
      return;
    }

    if (itimeout % 10 == 0)
      // check connection status every 5 seconds
      itimeout = ConnectionProcessTimer(itimeout);
  } else {
    static PeriodClock m_clock;
    if (m_clock.elapsed() < 0)
      m_clock.update();

    if (replay.Update()) {
      m_clock.update();
    } else if (m_clock.elapsed() >= 1000) {
      m_clock.update();
      device_blackboard.ProcessSimulation();
      TriggerGPSUpdate();
      device_blackboard.RaiseConnection();
    }
  }
}
