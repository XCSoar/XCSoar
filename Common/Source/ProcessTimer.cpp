/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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
#include "InputEvents.h"
#include "ReplayLogger.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Device/device.h"
#include "Device/Parser.h"
#include "Dialogs.h"
#include "Screen/Blank.hpp"
#include "SettingsUser.hpp"
#include "Message.h"
#include "UtilsSystem.hpp"
#include "InfoBoxManager.h"
#include "MapWindow.h"
#include "Math/Earth.hpp"
#include "Blackboard.hpp"
#include "Components.hpp"


static void HeapCompactTimer()
{
  static int iheapcompact = 0;
  // called 2 times per second, compact heap every minute.
  iheapcompact++;
  if (iheapcompact == 120) {
    MyCompactHeaps();
    iheapcompact = 0;
  }
}

void ProcessTimer::CommonProcessTimer()
{
  // service the GCE and NMEA queue
  if (globalRunningEvent.test()) {

    InputEvents::DoQueuedEvents();

    if (airspaceWarningEvent.test()) {
      airspaceWarningEvent.reset();
      ResetDisplayTimeOut();
      dlgAirspaceWarningShowDlg(RequestAirspaceWarningForce);
      RequestAirspaceWarningForce = false;
    }
    // update FLARM display (show/hide)
    if (gauge_flarm != NULL)
      gauge_flarm->Show();
  }
  SendSettingsComputer();
  SendSettingsMap();

  InfoBoxManager::ProcessTimer();

  InputEvents::ProcessMenuTimer();

  CheckDisplayTimeOut(false);

  // don't display messages if airspace warning dialog is active
  if (!dlgAirspaceWarningVisible()) {
    if (Message::Render()) {
      // turn screen on if blanked and receive a new message
      ResetDisplayTimeOut();
    }
  }

  HeapCompactTimer();
}

////////////////

int ProcessTimer::ConnectionProcessTimer(int itimeout) {
  mutexComm.Lock();
  NMEAParser::UpdateMonitor();
  mutexComm.Unlock();

  static bool connected_last = false;
  static bool wait_connect = false;
  static bool wait_lock = false;
  //
  // replace bool with BOOL to correct warnings and match variable
  // declarations RB
  //

  if (!Basic().Connected) {
    // if gps is not connected, set navwarning to true so
    // calculations flight timers don't get updated
    device_blackboard.SetNAVWarning(true);
  }
  bool connected_now = device_blackboard.LowerConnection();

  if (connected_now && Basic().NAVWarning) {

    if (!wait_lock) {
      // waiting for lock first time
      wait_lock = true;
      itimeout = 0;
      InputEvents::processGlideComputer(GCE_GPS_FIX_WAIT);
#ifndef DISABLEAUDIO
      MessageBeep(MB_ICONEXCLAMATION);
#endif
    }

    // If GPS connected but no lock, must be in hangar
    if (InterfaceTimeoutCheck()) {
#ifdef GNAV
      // TODO feature: ask question about shutdown or give warning
      // then shutdown if no activity.
      // Shutdown();
#endif
    }
  } else if (connected_now) { // !navwarning
    wait_connect = false;
    wait_lock = false;
    itimeout = 0;
  } else { // not connected
    wait_lock = false;
  }

  if(!connected_now && !connected_last) {
    // re-draw screen every five seconds even if no GPS
    TriggerGPSUpdate();

    devLinkTimeout(devAll());

    if(!wait_connect) {
      // gps is waiting for connection first time
      wait_connect = true;
      InputEvents::processGlideComputer(GCE_GPS_CONNECTION_WAIT);
#ifndef DISABLEAUDIO
      MessageBeep(MB_ICONEXCLAMATION);
#endif
    } else if (itimeout % 30 == 0) {
      itimeout = 0;
      // we've been waiting for connection a long time
      // no activity for 30 seconds, so assume PDA has been
      // switched off and on again
      //
#ifndef WINDOWSPC
#ifndef GNAV
      InputEvents::processGlideComputer(GCE_COMMPORT_RESTART);
      devRestart();
#endif
#endif
    }
  }

  connected_last = connected_now;
  return itimeout;
}


#ifndef _SIM_
void ProcessTimer::Process(void)
{

  if (!Basic().Connected && DisplayTimeOutIsFresh()) {
    // JMW 20071207
    // re-draw screen every five seconds even if no GPS
    // this prevents sluggish screen when inside hangar..
    TriggerGPSUpdate();
  }

  CommonProcessTimer();

  // now check GPS status
  devTick();

  static int itimeout = -1;
  itimeout++;

  // also service replay logger
  ReplayLogger::Update();
  if (ReplayLogger::IsEnabled()) {
    static double timeLast = 0;
    if (Basic().Time-timeLast>=1.0) {
      TriggerGPSUpdate();
    }
    timeLast = Basic().Time;
    device_blackboard.RaiseConnection();
    device_blackboard.SetNAVWarning(false);
    return;
  }

  if (itimeout % 10 == 0) {
    // check connection status every 5 seconds
    itimeout = ConnectionProcessTimer(itimeout);
  }
}
#endif // end processing of non-simulation mode


#ifdef _SIM_
void ProcessTimer::SIMProcess(void)
{

  CommonProcessTimer();

  device_blackboard.RaiseConnection();
  static int i=0;
  i++;

  if (!ReplayLogger::Update()) {
    if (i%2==0) return;
    device_blackboard.ProcessSimulation();
  }

  if (i%2==0) return;

  TriggerGPSUpdate();
}
#endif
