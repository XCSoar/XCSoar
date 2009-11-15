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
#include "Dialogs/Message.hpp"
#include "StatusMessage.hpp"
#include "InfoBoxManager.h"
#include "InfoBoxLayout.h"
#include "Asset.hpp"

static Mutex mutexInterfaceTimeout;
static int interface_timeout;
static bool doForceShutdown = false;
static bool ShutdownRequested = false;

InterfaceBlackboard CommonInterface::blackboard;
HINSTANCE CommonInterface::hInst; // The current instance
StatusMessageList CommonInterface::status_messages;
MainWindow CommonInterface::main_window(status_messages);

// settings used only by interface thread scope
bool CommonInterface::VirtualKeys=false;
bool ActionInterface::RequestAirspaceWarningForce = false;
bool ActionInterface::LockSettingsInFlight = true;
unsigned  ActionInterface::UserLevel=0; // used by dlgConfiguration
unsigned XCSoarInterface::debounceTimeout=200;
int ActionInterface::MenuTimeoutMax = MENUTIMEOUTMAX;
bool CommonInterface::EnableAutoBacklight=true;
bool CommonInterface::EnableAutoSoundVolume=true;
int CommonInterface::ActiveAlternate = -1;
bool CommonInterface::EnableAnimation = false;

#include "LogFile.hpp"
#include "Protection.hpp"
#include "DeviceBlackboard.hpp"

void XCSoarInterface::ExchangeBlackboard() {
  ScopeLock protect(mutexBlackboard);
  ReceiveBlackboard();
  ReceiveMapProjection();
  SendSettingsComputer();
  SendSettingsMap();
}

void XCSoarInterface::ReceiveBlackboard() {
  ScopeLock protect(mutexBlackboard);
  ReadBlackboardBasic(device_blackboard.Basic());
  ReadBlackboardCalculated(device_blackboard.Calculated());

  if (Calculated().TeammateCodeValid) {
    SetSettingsComputer().TeammateCodeValid= true;
  }
}

void ActionInterface::SendSettingsComputer() {
  ScopeLock protect(mutexBlackboard);
  // send computer settings to the device because we know
  // that it won't be reading from them if we lock it, and
  // then others can retrieve from it at their convenience.
  device_blackboard.ReadSettingsComputer(SettingsComputer());
  // TODO: trigger refresh if the settings are changed
}

void XCSoarInterface::ReceiveMapProjection()
{
  ScopeLock protect(mutexBlackboard);
  ReadMapProjection(device_blackboard.MapProjection());
}

/**
 * Send the own SettingsMap to the DeviceBlackboard
 * @param trigger_draw Triggers the draw event after sending if true
 */
void ActionInterface::SendSettingsMap(const bool trigger_draw) {
  // QUESTION TB: what is trigger_draw?
  ScopeLock protect(mutexBlackboard);

  if (trigger_draw) {
    DisplayModes();
    InfoBoxManager::ProcessTimer();
  }

  // Copy InterfaceBlackboard.SettingsMap to the DeviceBlackboard
  device_blackboard.ReadSettingsMap(SettingsMap());

  if (trigger_draw) {
    drawTriggerEvent.trigger();
  }
  // TODO: trigger refresh if the settings are changed
}

bool XCSoarInterface::InterfaceTimeoutZero(void) {
  ScopeLock protect(mutexInterfaceTimeout);
  return (interface_timeout==0);
}

void XCSoarInterface::InterfaceTimeoutReset(void) {
  ScopeLock protect(mutexInterfaceTimeout);
  interface_timeout = 0;
}

bool XCSoarInterface::InterfaceTimeoutCheck(void) {
  ScopeLock protect(mutexInterfaceTimeout);
  if (interface_timeout > 60*10) {
    interface_timeout = 0;
    return true;
  } else {
    interface_timeout++;
    return false;
  }
}

void ActionInterface::SignalShutdown(bool force) {
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

// Debounce input buttons (does not matter which button is pressed)
// VNT 090702 FIX Careful here: synthetic double clicks and virtual keys require some timing.
// See Defines.h DOUBLECLICKINTERVAL . Not sure they are 100% independent.

#include "PeriodClock.hpp"
#include "Screen/Blank.hpp"

bool XCSoarInterface::Debounce(void) {
  static PeriodClock fps_last;

  ResetDisplayTimeOut();
  InterfaceTimeoutReset();

  if (SettingsMap().ScreenBlanked) {
    // prevent key presses working if screen is blanked,
    // so a key press just triggers turning the display on again
    return false;
  }

  return fps_last.check_update(debounceTimeout);
}

/**
 * Determine whether the vario gauge should be drawn depending on the
 * display orientation and the infobox layout
 * @return True if vario gauge should be drawn, False otherwise
 */
bool vario_visible() {
  bool gaugeVarioInPortrait = is_altair();
  bool enable_gauge;

  // TODO TB: logic update...

  // VENTA3 disable gauge vario for geometry 5 in landscape mode, use 8
  // box right instead beside those boxes were painted and overwritten
  // by the gauge already and gauge was graphically too much stretched,
  // requiring a restyle!

  if (InfoBoxLayout::gnav) {
    if ((InfoBoxLayout::landscape == true) &&
        (InfoBoxLayout::InfoBoxGeometry == 5))
      enable_gauge = false;
    else
      enable_gauge = true;
  } else {
    enable_gauge = false;
  }

 // Disable vario gauge in geometry 5 landscape mode, leave 8 boxes on
 // the right
  if ((InfoBoxLayout::landscape == true)
      && (InfoBoxLayout::InfoBoxGeometry == 5)) return false; // VENTA3

  if (gaugeVarioInPortrait || InfoBoxLayout::landscape) {
    return enable_gauge;
  }
  return false;
}

#include "Gauge/GaugeVario.hpp"
#include "Gauge/GaugeFLARM.hpp"

/**
 * Determine whether vario gauge, FLARM radar and infoboxes should be drawn
 */
void
ActionInterface::DisplayModes()
{
  // Determine whether the vario gauge should be drawn
  SetSettingsMap().EnableVarioGauge =
    vario_visible() && !SettingsMap().FullScreen;

  if (main_window.vario) {
    if (!SettingsMap().ScreenBlanked && SettingsMap().EnableVarioGauge) {
      main_window.vario->show();
    } else {
      main_window.vario->hide();
    }
  }

  if (Basic().NewTraffic) {
    // TODO bug: JMW: broken, currently won't work very well, needs to be reworked
    GaugeFLARM *gauge_flarm = main_window.flarm;
    if (gauge_flarm != NULL)
      gauge_flarm->Suppress = false;
  }

  if (SettingsMap().FullScreen) {
    InfoBoxManager::Hide();
  } else {
    InfoBoxManager::Show();
  }
}
