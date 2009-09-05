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
#include "Interface.hpp"
#include "Thread/Mutex.hpp"
#include "MainWindow.hpp"
#include "Language.hpp"
#include "Dialogs.h"

bool LockSettingsInFlight = true;
int debounceTimeout=200;

static Mutex mutexInterfaceTimeout;
static int interface_timeout;
static bool doForceShutdown = false;
static bool ShutdownRequested = false;

///

InterfaceBlackboard CommonInterface::blackboard;
HINSTANCE CommonInterface::hInst; // The current instance
MainWindow CommonInterface::main_window;


void XCSoarInterface::DefaultSettings() 
{

  SetSettingsComputer().AutoWindMode= D_AUTOWIND_CIRCLING;
  SetSettingsComputer().AutoMcMode = 0;
}


void XCSoarInterface::SendSettings() {
  // send computer settings to the device because we know
  // that it won't be reading from them if we lock it, and
  // then others can retrieve from it at their convenience.
  mutexFlightData.Lock();
  device_blackboard.ReadSettingsComputer(SettingsComputer());
  mutexFlightData.Unlock();
  // TODO: trigger refresh if the settings are changed
}


bool XCSoarInterface::InterfaceTimeoutZero(void) {
  bool retval;
  mutexInterfaceTimeout.Lock();
  retval= (interface_timeout==0);
  mutexInterfaceTimeout.Unlock();
  return retval;
}

void XCSoarInterface::InterfaceTimeoutReset(void) {
  mutexInterfaceTimeout.Lock();
  interface_timeout = 0;
  mutexInterfaceTimeout.Unlock();
}


bool XCSoarInterface::InterfaceTimeoutCheck(void) {
  bool retval;
  mutexInterfaceTimeout.Lock();
  if (interface_timeout > 60*10) {
    interface_timeout = 0;
    retval= true;
  } else {
    interface_timeout++;
    retval= false;
  }
  mutexInterfaceTimeout.Unlock();
  return retval;
}

void XCSoarInterface::SignalShutdown(bool force) {
  if (!ShutdownRequested) {
    doForceShutdown = force;
    ShutdownRequested = true;
    main_window.close(); // signals close
  }
}

bool XCSoarInterface::CheckShutdown(void) {
  bool retval = false;
  if (ShutdownRequested) {
    if(doForceShutdown ||
       (MessageBoxX(gettext(TEXT("Quit program?")),
		    gettext(TEXT("XCSoar")),
		    MB_YESNO|MB_ICONQUESTION) == IDYES)) {
      retval = true;
    } else {
      retval = false;
    }
    doForceShutdown = false;
    ShutdownRequested = false;
  }
  return retval;
}


/////////////////////
// Debounce input buttons (does not matter which button is pressed)
// VNT 090702 FIX Careful here: synthetic double clicks and virtual keys require some timing.
// See Defines.h DOUBLECLICKINTERVAL . Not sure they are 100% independent.

#include "PeriodClock.hpp"
#include "Screen/Blank.hpp"

bool XCSoarInterface::Debounce(void) {
  static PeriodClock fps_last;

  ResetDisplayTimeOut();
  InterfaceTimeoutReset();

  if (ScreenBlanked) {
    // prevent key presses working if screen is blanked,
    // so a key press just triggers turning the display on again
    return false;
  }

  return fps_last.check_update(debounceTimeout);
}

