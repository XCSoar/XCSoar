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
#include "XCSoar.h"
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
#include "Utils.h"
#include "UtilsSystem.hpp"
#include "InfoBoxManager.h"
#include "MapWindow.h"
#include "Math/Earth.hpp"
#include "Blackboard.hpp"

BOOL GPSCONNECT = FALSE;
BOOL extGPSCONNECT = FALSE; // this one used by extrnal functions

bool RequestAirspaceWarningForce=false;

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

void CommonProcessTimer()
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
    GaugeFLARM::Show();
  }

  InfoBoxProcessTimer();

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


int ConnectionProcessTimer(int itimeout) {
  mutexComm.Lock();
  NMEAParser::UpdateMonitor();
  mutexComm.Unlock();

  static BOOL LastGPSCONNECT = FALSE;
  static BOOL CONNECTWAIT = FALSE;
  static BOOL LOCKWAIT = FALSE;

  //
  // replace bool with BOOL to correct warnings and match variable
  // declarations RB
  //
  BOOL gpsconnect = GPSCONNECT;

  if (GPSCONNECT) {
    extGPSCONNECT = TRUE;
  }

  if (!extGPSCONNECT) {
    // if gps is not connected, set navwarning to true so
    // calculations flight timers don't get updated
    mutexFlightData.Lock();
    GPS_INFO.NAVWarning = true;
    mutexFlightData.Unlock();
  }

  GPSCONNECT = FALSE;
  BOOL navwarning = (BOOL)(GPS_INFO.NAVWarning);

  if (gpsconnect && navwarning) {
    // If GPS connected but no lock, must be in hangar
    if (InterfaceTimeoutCheck()) {
#ifdef GNAV
      // TODO feature: ask question about shutdown or give warning
      // then shutdown if no activity.
      //     Shutdown();
#endif
    }
  }

  if((gpsconnect == FALSE) && (LastGPSCONNECT == FALSE))
    {
      // re-draw screen every five seconds even if no GPS
      TriggerGPSUpdate();

      devLinkTimeout(devAll());

      if(LOCKWAIT == TRUE)
	{
	  // gps was waiting for fix, now waiting for connection
	  LOCKWAIT = FALSE;
	}
      if(!CONNECTWAIT)
	{
	  // gps is waiting for connection first time

	  extGPSCONNECT = FALSE;
          InputEvents::processGlideComputer(GCE_GPS_CONNECTION_WAIT);

	  CONNECTWAIT = TRUE;
#ifndef DISABLEAUDIO
	  MessageBeep(MB_ICONEXCLAMATION);
#endif
	} else {

	if (itimeout % 30 == 0) {
	  // we've been waiting for connection a long time

	  // no activity for 30 seconds, so assume PDA has been
	  // switched off and on again
	  //
#if (WINDOWSPC<1)
#ifndef GNAV

	  extGPSCONNECT = FALSE;

	  InputEvents::processGlideComputer(GCE_COMMPORT_RESTART);

	  RestartCommPorts();
#endif
#endif

#if (EXPERIMENTAL > 0)
	  // if comm port shut down, probably so did bluetooth dialup
	  // so restart it here also.
	  bsms.Shutdown();
	  bsms.Initialise();
#endif

	  itimeout = 0;
	}
      }
    }

  if((gpsconnect == TRUE) && (LastGPSCONNECT == FALSE))
    {
      itimeout = 0; // reset timeout

      if(CONNECTWAIT)
	{
	  TriggerGPSUpdate();
	  CONNECTWAIT = FALSE;
	}
    }

  if((gpsconnect == TRUE) && (LastGPSCONNECT == TRUE))
    {
      if((navwarning == TRUE) && (LOCKWAIT == FALSE))
	{
	  InputEvents::processGlideComputer(GCE_GPS_FIX_WAIT);

	  TriggerGPSUpdate();

	  LOCKWAIT = TRUE;
#ifndef DISABLEAUDIO
	  MessageBeep(MB_ICONEXCLAMATION);
#endif
	}
      else if((navwarning == FALSE) && (LOCKWAIT == TRUE))
	{
	  TriggerGPSUpdate();
	  LOCKWAIT = FALSE;
	}
    }

  LastGPSCONNECT = gpsconnect;
  return itimeout;
}


#ifndef _SIM_
void ProcessTimer(void)
{

  if (!GPSCONNECT && DisplayTimeOutIsFresh()) {
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
    if (GPS_INFO.Time-timeLast>=1.0) {
      TriggerGPSUpdate();
    }
    timeLast = GPS_INFO.Time;
    GPSCONNECT = TRUE;
    extGPSCONNECT = TRUE;
    GPS_INFO.NAVWarning = FALSE;
    GPS_INFO.SatellitesUsed = 6;
    return;
  }

  if (itimeout % 10 == 0) {
    // check connection status every 5 seconds
    itimeout = ConnectionProcessTimer(itimeout);
  }
}
#endif // end processing of non-simulation mode


#ifdef _SIM_
void SIMProcessTimer(void)
{

  CommonProcessTimer();

  GPSCONNECT = TRUE;
  extGPSCONNECT = TRUE;
  static int i=0;
  i++;

  if (!ReplayLogger::Update()) {

    if (i%2==0) return;

    mutexFlightData.Lock();

    GPS_INFO.NAVWarning = FALSE;
    GPS_INFO.SatellitesUsed = 6;
    FindLatitudeLongitude(GPS_INFO.Latitude, GPS_INFO.Longitude,
                          GPS_INFO.TrackBearing, GPS_INFO.Speed*1.0,
                          &GPS_INFO.Latitude,
                          &GPS_INFO.Longitude);
    GPS_INFO.Time+= 1.0;
    long tsec = (long)GPS_INFO.Time;
    GPS_INFO.Hour = tsec/3600;
    GPS_INFO.Minute = (tsec-GPS_INFO.Hour*3600)/60;
    GPS_INFO.Second = (tsec-GPS_INFO.Hour*3600-GPS_INFO.Minute*60);

    mutexFlightData.Unlock();
  }

  if (i%2==0) return;

#ifndef NDEBUG
  // use this to test FLARM parsing/display
#ifndef GNAV
  NMEAParser::TestRoutine(&GPS_INFO);
#endif
#endif

  TriggerGPSUpdate();
}
#endif
