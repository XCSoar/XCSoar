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

/*

InputEvents

This class is used to control all user and external InputEvents.
This includes some Nmea strings, virtual events (Glide Computer
Evnets) and Keyboard.

What it does not cover is Glide Computer normal processing - this
includes GPS and Vario processing.

What it does include is what to do when an automatic event (switch
to Climb mode) and user events are entered.

It also covers the configuration side of on screen labels.

For further information on config file formats see

source/Common/Data/Input/ALL
doc/html/advanced/input/ALL		http://xcsoar.sourceforge.net/advanced/input/

*/

#include "InputEvents.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "Device/Parser.h"
#include "Blackboard.hpp"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "SettingsUser.hpp"
#include "Math/FastMath.h"
#include "Dialogs.h"
#include "Message.h"
#include "Marks.h"
#include <commctrl.h>
#include "Airspace.h"
#include "InfoBoxLayout.h"
#include "InfoBoxManager.h"
#include "InfoBoxEvents.h"
#include "Device/device.h"
#include "Message.h"
#include "Units.hpp"
#include "MapWindow.h"
#include "Atmosphere.h"
#include "Gauge/GaugeFLARM.hpp"
#include "Waypointparser.h"
#include "Registry.hpp"
#include "LocalPath.hpp"
#include "UtilsProfile.hpp"
#include "UtilsText.hpp"
#include "UtilsSystem.hpp"
#include "McReady.h"
#include "Interface.hpp"
#include "Calculations.h" // TODO danger! ClearAirspaceWarnings

#ifdef PNA
#include "Asset.hpp"
#endif

#include <assert.h>
#include <ctype.h>
#include <tchar.h>



// DLL Cache
typedef void (CALLBACK *DLLFUNC_INPUTEVENT)(TCHAR*);
typedef void (CALLBACK *DLLFUNC_SETHINST)(HMODULE);


#define MAX_DLL_CACHE 256
typedef struct {
  TCHAR *text;
  HINSTANCE hinstance;
} DLLCACHESTRUCT;
DLLCACHESTRUCT DLLCache[MAX_DLL_CACHE];
int DLLCache_Count = 0;


// -----------------------------------------------------------------------
// Execution - list of things you can do
// -----------------------------------------------------------------------

// TODO code: Keep marker text for use in log file etc.
void InputEvents::eventMarkLocation(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("reset")) == 0) {
    Marks::reset_marks = true;
  } else {
    mutexComm.Lock(); // Must LockComm to prevent deadlock
    mutexFlightData.Lock();
    Marks::MarkLocation(GPS_INFO.Longitude, GPS_INFO.Latitude);
    mutexFlightData.Unlock();
    mutexComm.Unlock();
  }
}


void InputEvents::eventSounds(const TCHAR *misc) {
 // bool OldEnableSoundVario = EnableSoundVario;

  if (_tcscmp(misc, TEXT("toggle")) == 0)
    EnableSoundVario = !EnableSoundVario;
  else if (_tcscmp(misc, TEXT("on")) == 0)
    EnableSoundVario = true;
  else if (_tcscmp(misc, TEXT("off")) == 0)
    EnableSoundVario = false;
  else if (_tcscmp(misc, TEXT("show")) == 0) {
    if (EnableSoundVario)
      Message::AddMessage(TEXT("Vario Sounds ON"));
    else
      Message::AddMessage(TEXT("Vario Sounds OFF"));
  }
  /*
  if (EnableSoundVario != OldEnableSoundVario) {
    VarioSound_EnableSound(EnableSoundVario);
  }
  */
}

void InputEvents::eventSnailTrail(const TCHAR *misc) {

  if (_tcscmp(misc, TEXT("toggle")) == 0) {
    TrailActive ++;
    if (TrailActive>3) {
      TrailActive=0;
    }
  }
  else if (_tcscmp(misc, TEXT("off")) == 0)
    TrailActive = 0;
  else if (_tcscmp(misc, TEXT("long")) == 0)
    TrailActive = 1;
  else if (_tcscmp(misc, TEXT("short")) == 0)
    TrailActive = 2;
  else if (_tcscmp(misc, TEXT("full")) == 0)
    TrailActive = 3;

  else if (_tcscmp(misc, TEXT("show")) == 0) {
    if (TrailActive==0)
      Message::AddMessage(TEXT("SnailTrail OFF"));
    if (TrailActive==1)
      Message::AddMessage(TEXT("SnailTrail ON Long"));
    if (TrailActive==2)
      Message::AddMessage(TEXT("SnailTrail ON Short"));
    if (TrailActive==3)
      Message::AddMessage(TEXT("SnailTrail ON Full"));
  }
}

// VENTA3
void InputEvents::eventVisualGlide(const TCHAR *misc) {

  if (_tcscmp(misc, TEXT("toggle")) == 0) {
    VisualGlide ++;
    if (VisualGlide==2 && !ExtendedVisualGlide) VisualGlide=0;
    if (VisualGlide>2) {
      VisualGlide=0;
    }
  }
  else if (_tcscmp(misc, TEXT("off")) == 0)
    VisualGlide = 0;
  else if (_tcscmp(misc, TEXT("steady")) == 0)
    VisualGlide = 1;
  else if (_tcscmp(misc, TEXT("moving")) == 0)
    VisualGlide = 2;

  else if (_tcscmp(misc, TEXT("show")) == 0) {
    if (VisualGlide==0)
      Message::AddMessage(TEXT("VisualGlide OFF"));
    if (VisualGlide==1)
      Message::AddMessage(TEXT("VisualGlide Steady"));
    if (VisualGlide==2)
      Message::AddMessage(TEXT("VisualGlide Moving"));
  }
}

// VENTA3
/*
 * This even currently toggles DrawAirSpace() and does nothing else.
 * But since we use an int and not a bool, it is easy to expand it.
 * Note that XCSoar.cpp init OnAirSpace always to 1, and this value
 * is never saved to the registry actually. It is intended to be used
 * as a temporary choice during flight, does not affect configuration.
 * Note also that in MapWindow DrawAirSpace() is accomplished for
 * every OnAirSpace value >0 .  We can use negative numbers also,
 * but 0 should mean OFF all the way.
 */
void InputEvents::eventAirSpace(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("toggle")) == 0) {
    OnAirSpace ++;
    if (OnAirSpace>1) {
      OnAirSpace=0;
    }
  }
  else if (_tcscmp(misc, TEXT("off")) == 0)
    OnAirSpace = 0;
  else if (_tcscmp(misc, TEXT("on")) == 0)
    OnAirSpace = 1;
  else if (_tcscmp(misc, TEXT("show")) == 0) {
    if (OnAirSpace==0)
      Message::AddMessage(TEXT("Show AirSpace OFF"));
    if (OnAirSpace==1)
      Message::AddMessage(TEXT("Show AirSpace ON"));
  }
}

void InputEvents::eventScreenModes(const TCHAR *misc) {
  // toggle switches like this:
  //  -- normal infobox
  //  -- auxiliary infobox
  //  -- full screen
  //  -- normal infobox

  if (_tcscmp(misc, TEXT("normal")) == 0) {
    map_window.RequestFullScreen(false);
    EnableAuxiliaryInfo = false;
  } else if (_tcscmp(misc, TEXT("auxilary")) == 0) {
    map_window.RequestFullScreen(false);
    EnableAuxiliaryInfo = true;
  } else if (_tcscmp(misc, TEXT("toggleauxiliary")) == 0) {
    map_window.RequestFullScreen(false);
    EnableAuxiliaryInfo = !EnableAuxiliaryInfo;
  } else if (_tcscmp(misc, TEXT("full")) == 0) {
    map_window.RequestFullScreen(true);
  } else if (_tcscmp(misc, TEXT("togglefull")) == 0) {
    map_window.RequestToggleFullScreen();
  } else if (_tcscmp(misc, TEXT("show")) == 0) {
    if (map_window.isMapFullScreen())
      Message::AddMessage(TEXT("Screen Mode Full"));
    else if (EnableAuxiliaryInfo)
      Message::AddMessage(TEXT("Screen Mode Auxiliary"));
    else
      Message::AddMessage(TEXT("Screen Mode Normal"));
  } else if (_tcscmp(misc, TEXT("togglebiginfo")) == 0) {
    InfoBoxLayout::fullscreen = !InfoBoxLayout::fullscreen;
  } else {

  //
  // VENTA-ADDON TOGGLE SCROLLWHEEL as INPUT on the HP31X
  //

#ifdef PNA

    if ( GlobalModelType == MODELTYPE_PNA_HP31X ) {
      // 1 normal > 2 aux > 3 biginfo > 4 fullscreen
      short pnascrollstatus;
      pnascrollstatus=1;
      if ( InfoBoxLayout::fullscreen == true ) pnascrollstatus=3;
      if ( map_window.isMapFullScreen() ) pnascrollstatus=4;
      if ( EnableAuxiliaryInfo == true ) pnascrollstatus=2;

      switch (pnascrollstatus) {
      case 1:
#ifndef DISABLEAUDIO
	if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
#endif
	EnableAuxiliaryInfo = true;
	break;
      case 2:
	//	EnableAuxiliaryInfo = false;		// Disable BigInfo until it is useful
	//	InfoBoxLayout::fullscreen = true;
	//	break;
      case 3:
	//	InfoBoxLayout::fullscreen = false;
#ifndef DISABLEAUDIO
	if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
#endif
	EnableAuxiliaryInfo = false;
	map_window.RequestFullScreen(true);
	break;
      case 4:
	//	InfoBoxLayout::fullscreen = false;
	EnableAuxiliaryInfo = false;
	map_window.RequestFullScreen(false);
#ifndef DISABLEAUDIO
	if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_BELL"));
#endif
	break;
      default:
	break;
      } // switch pnascrollstatus
    } // not a PNA_HP31X
    else
      {
	if (EnableAuxiliaryInfo) {
#ifndef DISABLEAUDIO
	  if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
#endif
	  map_window.RequestToggleFullScreen();
	  EnableAuxiliaryInfo = false;
	} else {
	  if (map_window.isMapFullScreen()) {
	    map_window.RequestToggleFullScreen();
#ifndef DISABLEAUDIO
	    if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_BELL"));
#endif
	  } else {
#ifndef DISABLEAUDIO
	    if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
#endif
	    EnableAuxiliaryInfo = true;
	  }
	}
      } // fallback for other PNAs

#else // UNDEFINED PNA
    if (EnableAuxiliaryInfo) {
#ifndef DISABLEAUDIO
      if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
#endif
      map_window.RequestToggleFullScreen();
      EnableAuxiliaryInfo = false;
    } else {
      if (map_window.isMapFullScreen()) {
	map_window.RequestToggleFullScreen();

#ifndef DISABLEAUDIO  // VENTA-ADDON SOUND CYCLING SCREENS
	if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_BELL"));
#endif
      } else {
#ifndef DISABLEAUDIO
	if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_CLICK"));
#endif
	EnableAuxiliaryInfo = true;
      }
    }
#endif // def/undef PNA
  }
}


// eventAutoZoom - Turn on|off|toggle AutoZoom
// misc:
//	auto on - Turn on if not already
//	auto off - Turn off if not already
//	auto toggle - Toggle current full screen status
//	auto show - Shows autozoom status
//	+	- Zoom in
//	++	- Zoom in near
//	-	- Zoom out
//	--	- Zoom out far
//	n.n	- Zoom to a set scale
//	show - Show current zoom scale
void InputEvents::eventZoom(const TCHAR* misc) {
  // JMW pass through to handler in MapWindow
  // here:
  // -1 means toggle
  // 0 means off
  // 1 means on
  float zoom;

  if (_tcscmp(misc, TEXT("auto toggle")) == 0)
    map_window.Event_AutoZoom(-1);
  else if (_tcscmp(misc, TEXT("auto on")) == 0)
    map_window.Event_AutoZoom(1);
  else if (_tcscmp(misc, TEXT("auto off")) == 0)
    map_window.Event_AutoZoom(0);
  else if (_tcscmp(misc, TEXT("auto show")) == 0) {
    if (AutoZoom)
      Message::AddMessage(TEXT("AutoZoom ON"));
    else
      Message::AddMessage(TEXT("AutoZoom OFF"));
  }
  else if (_tcscmp(misc, TEXT("slowout")) == 0)
    map_window.Event_ScaleZoom(-4);
  else if (_tcscmp(misc, TEXT("slowin")) == 0)
    map_window.Event_ScaleZoom(4);
  else if (_tcscmp(misc, TEXT("out")) == 0)
    map_window.Event_ScaleZoom(-1);
  else if (_tcscmp(misc, TEXT("in")) == 0)
    map_window.Event_ScaleZoom(1);
  else if (_tcscmp(misc, TEXT("-")) == 0)
    map_window.Event_ScaleZoom(-1);
  else if (_tcscmp(misc, TEXT("+")) == 0)
    map_window.Event_ScaleZoom(1);
  else if (_tcscmp(misc, TEXT("--")) == 0)
    map_window.Event_ScaleZoom(-2);
  else if (_tcscmp(misc, TEXT("++")) == 0)
    map_window.Event_ScaleZoom(2);
  else if (_stscanf(misc, TEXT("%f"), &zoom) == 1)
    map_window.Event_SetZoom((double)zoom);

  else if (_tcscmp(misc, TEXT("circlezoom toggle")) == 0) {
    CircleZoom = !CircleZoom;
  } else if (_tcscmp(misc, TEXT("circlezoom on")) == 0) {
    CircleZoom = true;
  } else if (_tcscmp(misc, TEXT("circlezoom off")) == 0) {
    CircleZoom = false;
  } else if (_tcscmp(misc, TEXT("circlezoom show")) == 0) {
    if (CircleZoom)
      Message::AddMessage(TEXT("Circling Zoom ON"));
    else
      Message::AddMessage(TEXT("Circling Zoom OFF"));
  }

}

// Pan
//	on	Turn pan on
//	off	Turn pan off
//      supertoggle Toggles pan and fullscreen
//	up	Pan up
//	down	Pan down
//	left	Pan left
//	right	Pan right
//	TODO feature: n,n	Go that direction - +/-
//	TODO feature: ???	Go to particular point
//	TODO feature: ???	Go to waypoint (eg: next, named)
void InputEvents::eventPan(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("toggle")) == 0)
    map_window.Event_Pan(-1);
  else if (_tcscmp(misc, TEXT("supertoggle")) == 0)
    map_window.Event_Pan(-2);
  else if (_tcscmp(misc, TEXT("on")) == 0)
    map_window.Event_Pan(1);
  else if (_tcscmp(misc, TEXT("off")) == 0)
    map_window.Event_Pan(0);

#if defined(PNA) || defined(FIVV)   // VENTA-ADDON  let pan mode scroll wheel zooming with HP31X. VENTA-TODO: make it different for other PNAs
 else if (_tcscmp(misc, TEXT("up")) == 0)
			map_window.Event_ScaleZoom(1);
else if (_tcscmp(misc, TEXT("down")) == 0)
			map_window.Event_ScaleZoom(-1); // fixed v58
#else
  else if (_tcscmp(misc, TEXT("up")) == 0)
    map_window.Event_PanCursor(0,1);
  else if (_tcscmp(misc, TEXT("down")) == 0)
    map_window.Event_PanCursor(0,-1);
#endif   // END VENTA
  else if (_tcscmp(misc, TEXT("left")) == 0)
    map_window.Event_PanCursor(1,0);
  else if (_tcscmp(misc, TEXT("right")) == 0)
    map_window.Event_PanCursor(-1,0);
  else if (_tcscmp(misc, TEXT("show")) == 0) {
    if (map_window.isPan())
      Message::AddMessage(TEXT("Pan mode ON"));
    else
      Message::AddMessage(TEXT("Pan mode OFF"));
  }

}

// Do JUST Terrain/Toplogy (toggle any, on/off any, show)
void InputEvents::eventTerrainTopology(const TCHAR *misc) {

  if (_tcscmp(misc, TEXT("terrain toggle")) == 0)
    map_window.Event_TerrainTopology(-2);

  else if (_tcscmp(misc, TEXT("toplogy toggle")) == 0)
    map_window.Event_TerrainTopology(-3);

  else if (_tcscmp(misc, TEXT("terrain on")) == 0)
    map_window.Event_TerrainTopology(3);

  else if (_tcscmp(misc, TEXT("terrain off")) == 0)
    map_window.Event_TerrainTopology(4);

  else if (_tcscmp(misc, TEXT("topology on")) == 0)
    map_window.Event_TerrainTopology(1);

  else if (_tcscmp(misc, TEXT("topology off")) == 0)
    map_window.Event_TerrainTopology(2);

  else if (_tcscmp(misc, TEXT("show")) == 0)
    map_window.Event_TerrainTopology(0);

  else if (_tcscmp(misc, TEXT("toggle")) == 0)
    map_window.Event_TerrainTopology(-1);

}

// Do clear warnings IF NONE Toggle Terrain/Topology
void InputEvents::eventClearWarningsOrTerrainTopology(const TCHAR *misc) {
	(void)misc;
  if (ClearAirspaceWarnings(true,false)) {
    // airspace was active, enter was used to acknowledge
    return;
  }
  // Else toggle TerrainTopology - and show the results
  map_window.Event_TerrainTopology(-1);
  map_window.Event_TerrainTopology(0);
}

// ClearAirspaceWarnings
// Clears airspace warnings for the selected airspace
//     day: clears the warnings for the entire day
//     ack: clears the warnings for the acknowledgement time
void InputEvents::eventClearAirspaceWarnings(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("day")) == 0)
    // JMW clear airspace warnings for entire day (for selected airspace)
    ClearAirspaceWarnings(true,true);
  else {

    // default, clear airspace for short acknowledgement time
    if (ClearAirspaceWarnings(true,false)) {

    }
  }
}

// ClearStatusMessages
// Do Clear Event Warnings
void InputEvents::eventClearStatusMessages(const TCHAR *misc) {
	(void)misc;
  // TODO enhancement: allow selection of specific messages (here we are acknowledging all)
  Message::Acknowledge(0);
}

void InputEvents::eventFLARMRadar(const TCHAR *misc) {
	(void)misc;
  //  if (_tcscmp(misc, TEXT("on")) == 0) {

  if (gauge_flarm == NULL)
    return;

  if (_tcscmp(misc, TEXT("ForceToggle")) == 0) {
    gauge_flarm->ForceVisible = !gauge_flarm->ForceVisible;
    EnableFLARMGauge = gauge_flarm->ForceVisible;
  } else
    gauge_flarm->Suppress = !gauge_flarm->Suppress;
  // the result of this will get triggered by refreshslots
}


// SelectInfoBox
// Selects the next or previous infobox
void InputEvents::eventSelectInfoBox(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("next")) == 0) {
    InfoBoxManager::Event_Select(1);
  }
  if (_tcscmp(misc, TEXT("previous")) == 0) {
    InfoBoxManager::Event_Select(-1);
  }
}

// ChangeInfoBoxType
// Changes the type of the current infobox to the next/previous type
void InputEvents::eventChangeInfoBoxType(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("next")) == 0) {
    InfoBoxManager::Event_Change(1);
  }
  if (_tcscmp(misc, TEXT("previous")) == 0) {
    InfoBoxManager::Event_Change(-1);
  }
}

// ArmAdvance
// Controls waypoint advance trigger:
//     on: Arms the advance trigger
//    off: Disarms the advance trigger
//   toggle: Toggles between armed and disarmed.
//   show: Shows current armed state
void InputEvents::eventArmAdvance(const TCHAR *misc) {
  if (AutoAdvance>=2) {
    if (_tcscmp(misc, TEXT("on")) == 0) {
      AdvanceArmed = true;
    }
    if (_tcscmp(misc, TEXT("off")) == 0) {
      AdvanceArmed = false;
    }
    if (_tcscmp(misc, TEXT("toggle")) == 0) {
      AdvanceArmed = !AdvanceArmed;
    }
  }
  if (_tcscmp(misc, TEXT("show")) == 0) {
    switch (AutoAdvance) {
    case 0:
      Message::AddMessage(TEXT("Auto Advance: Manual"));
      break;
    case 1:
      Message::AddMessage(TEXT("Auto Advance: Automatic"));
      break;
    case 2:
      if (AdvanceArmed) {
        Message::AddMessage(TEXT("Auto Advance: ARMED"));
      } else {
        Message::AddMessage(TEXT("Auto Advance: DISARMED"));
      }
      break;
    case 3:
      if (ActiveWayPoint<2) { // past start (but can re-start)
        if (AdvanceArmed) {
          Message::AddMessage(TEXT("Auto Advance: ARMED"));
        } else {
          Message::AddMessage(TEXT("Auto Advance: DISARMED"));
        }
      } else {
        Message::AddMessage(TEXT("Auto Advance: Automatic"));
      }
      break;
    default:
      break;
    }
  }
}

// DoInfoKey
// Performs functions associated with the selected infobox
//    up: triggers the up event
//    etc.
//    Functions associated with the infoboxes are described in the
//    infobox section in the reference guide
void InputEvents::eventDoInfoKey(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("up")) == 0) {
    InfoBoxManager::ProcessKey(1);
  }
  if (_tcscmp(misc, TEXT("down")) == 0) {
    InfoBoxManager::ProcessKey(-1);
  }
  if (_tcscmp(misc, TEXT("left")) == 0) {
    InfoBoxManager::ProcessKey(-2);
  }
  if (_tcscmp(misc, TEXT("right")) == 0) {
    InfoBoxManager::ProcessKey(2);
  }
  if (_tcscmp(misc, TEXT("return")) == 0) {
    InfoBoxManager::ProcessKey(0);
  }

}

// Mode
// Sets the current event mode.
//  The argument is the label of the mode to activate.
//  This is used to activate menus/submenus of buttons
void InputEvents::eventMode(const TCHAR *misc) {
  assert(misc != NULL);
  InputEvents::setMode(misc);

  // trigger redraw of screen to reduce blank area under windows
  drawTriggerEvent.trigger();
}

// Don't think we need this.
void InputEvents::eventMainMenu(const TCHAR *misc) {
	(void)misc;
  // todo: popup main menu
}

// Checklist
// Displays the checklist dialog
//  See the checklist dialog section of the reference manual for more info.
void InputEvents::eventChecklist(const TCHAR *misc) {
	(void)misc;
  dlgChecklistShowModal();
}

// FLARM Traffic
// Displays the FLARM traffic dialog
//  See the checklist dialog section of the reference manual for more info.
void InputEvents::eventFlarmTraffic(const TCHAR *misc) {
	(void)misc;
  dlgFlarmTrafficShowModal();
}


// Displays the task calculator dialog
//  See the task calculator dialog section of the reference manual
// for more info.
void InputEvents::eventCalculator(const TCHAR *misc) {
	(void)misc;
  dlgTaskCalculatorShowModal();
}

// Status
// Displays one of the three status dialogs:
//    system: display the system status
//    aircraft: displays the aircraft status
//    task: displays the task status
//  See the status dialog section of the reference manual for more info
//  on these.
void InputEvents::eventStatus(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("system")) == 0) {
    dlgStatusShowModal(1);
  } else if (_tcscmp(misc, TEXT("task")) == 0) {
    dlgStatusShowModal(2);
  } else if (_tcscmp(misc, TEXT("Aircraft")) == 0) {
    dlgStatusShowModal(0);
  } else {
    dlgStatusShowModal(-1);
  }
}

// Analysis
// Displays the analysis/statistics dialog
//  See the analysis dialog section of the reference manual
// for more info.
void InputEvents::eventAnalysis(const TCHAR *misc) {
	(void)misc;
  PopupAnalysis();
}

// WaypointDetails
// Displays waypoint details
//         current: the current active waypoint
//          select: brings up the waypoint selector, if the user then
//                  selects a waypoint, then the details dialog is shown.
//  See the waypoint dialog section of the reference manual
// for more info.
void InputEvents::eventWaypointDetails(const TCHAR *misc) {

  if (_tcscmp(misc, TEXT("current")) == 0) {
    mutexTaskData.Lock();
    if (ValidTaskPoint(ActiveWayPoint)) {
      SelectedWaypoint = Task[ActiveWayPoint].Index;
    }
    mutexTaskData.Unlock();
    if (SelectedWaypoint<0){
      Message::AddMessage(TEXT("No Active Waypoint!"));
      return;
    }
    PopupWaypointDetails();
  } else
    if (_tcscmp(misc, TEXT("select")) == 0) {
      int res = dlgWayPointSelect();

      if (res != -1){
	SelectedWaypoint = res;
	PopupWaypointDetails();
      };

    }
}


void InputEvents::eventGotoLookup(const TCHAR *misc) {
  int res = dlgWayPointSelect();
  if (res != -1){
    FlyDirectTo(res);
  };
}


// StatusMessage
// Displays a user defined status message.
//    The argument is the text to be displayed.
//    No punctuation characters are allowed.
void InputEvents::eventStatusMessage(const TCHAR *misc) {
  Message::AddMessage(misc);
}

// Plays a sound from the filename
void InputEvents::eventPlaySound(const TCHAR *misc) {
  PlayResource(misc);
}

// MacCready
// Adjusts MacCready settings
// up, down, auto on, auto off, auto toggle, auto show
void InputEvents::eventMacCready(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("up")) == 0) {
    MacCreadyProcessing(1);
  } else if (_tcscmp(misc, TEXT("down")) == 0) {
    MacCreadyProcessing(-1);
  } else if (_tcscmp(misc, TEXT("auto toggle")) == 0) {
    MacCreadyProcessing(0);
  } else if (_tcscmp(misc, TEXT("auto on")) == 0) {
    MacCreadyProcessing(+2);
  } else if (_tcscmp(misc, TEXT("auto off")) == 0) {
    MacCreadyProcessing(-2);
  } else if (_tcscmp(misc, TEXT("auto show")) == 0) {
    if (CALCULATED_INFO.AutoMacCready) {
      Message::AddMessage(TEXT("Auto MacCready ON"));
    } else {
      Message::AddMessage(TEXT("Auto MacCready OFF"));
    }
  } else if (_tcscmp(misc, TEXT("show")) == 0) {
    TCHAR Temp[100];
    _stprintf(Temp,TEXT("%0.1f"),GlidePolar::GetMacCready()*LIFTMODIFY);
    Message::AddMessage(TEXT("MacCready "), Temp);
  }
}


// Allows forcing of flight mode (final glide)
void InputEvents::eventFlightMode(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("finalglide on")) == 0) {
    ForceFinalGlide = true;
  }
  if (_tcscmp(misc, TEXT("finalglide off")) == 0) {
    ForceFinalGlide = false;
  }
  if (_tcscmp(misc, TEXT("finalglide toggle")) == 0) {
    ForceFinalGlide = !ForceFinalGlide;
  }
  if (_tcscmp(misc, TEXT("show")) == 0) {
    if (ForceFinalGlide) {
      Message::AddMessage(TEXT("Final glide forced ON"));
    } else {
      Message::AddMessage(TEXT("Final glide automatic"));
    }
  }
  if (ForceFinalGlide && ActiveWayPoint == -1){
    Message::AddMessage(TEXT("No Active Waypoint!"));
  }
}



// Wind
// Adjusts the wind magnitude and direction
//     up: increases wind magnitude
//   down: decreases wind magnitude
//   left: rotates wind direction counterclockwise
//  right: rotates wind direction clockwise
//   save: saves wind value, so it is used at next startup
//
// TODO feature: Increase wind by larger amounts ? Set wind to specific amount ?
//	(may sound silly - but future may get SMS event that then sets wind)
void InputEvents::eventWind(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("up")) == 0) {
    WindSpeedProcessing(1);
  }
  if (_tcscmp(misc, TEXT("down")) == 0) {
    WindSpeedProcessing(-1);
  }
  if (_tcscmp(misc, TEXT("left")) == 0) {
    WindSpeedProcessing(-2);
  }
  if (_tcscmp(misc, TEXT("right")) == 0) {
    WindSpeedProcessing(2);
  }
  if (_tcscmp(misc, TEXT("save")) == 0) {
    WindSpeedProcessing(0);
  }
}


int jmw_demo=0;

// SendNMEA
//  Sends a user-defined NMEA string to an external instrument.
//   The string sent is prefixed with the start character '$'
//   and appended with the checksum e.g. '*40'.  The user needs only
//   to provide the text in between the '$' and '*'.
//
void InputEvents::eventSendNMEA(const TCHAR *misc) {
  if (misc) {
    VarioWriteNMEA(misc);
  }
}

void InputEvents::eventSendNMEAPort1(const TCHAR *misc) {
  if (misc) {
    Port1WriteNMEA(misc);
  }
}

void InputEvents::eventSendNMEAPort2(const TCHAR *misc) {
  if (misc) {
    Port2WriteNMEA(misc);
  }
}

// AdjustVarioFilter
// When connected to the Vega variometer, this adjusts
// the filter time constant
//     slow/medium/fast
// The following arguments can be used for diagnostics purposes
//     statistics:
//     diagnostics:
//     psraw:
//     switch:
// The following arguments can be used to trigger demo modes:
//     climbdemo:
//     stfdemo:
// Other arguments control vario setup:
//     save: saves the vario configuration to nonvolatile memory on the instrument
//     zero: Zero's the airspeed indicator's offset
//
void InputEvents::eventAdjustVarioFilter(const TCHAR *misc) {
  static int naccel=0;
  if (_tcscmp(misc, TEXT("slow")) == 0) {
    VarioWriteNMEA(TEXT("PDVSC,S,VarioTimeConstant,3"));
    return;
  }
  if (_tcscmp(misc, TEXT("medium")) == 0) {
    VarioWriteNMEA(TEXT("PDVSC,S,VarioTimeConstant,2"));
    return;
  }
  if (_tcscmp(misc, TEXT("fast")) == 0) {
    VarioWriteNMEA(TEXT("PDVSC,S,VarioTimeConstant,1"));
    return;
  }
  if (_tcscmp(misc, TEXT("statistics"))==0) {
    VarioWriteNMEA(TEXT("PDVSC,S,Diagnostics,1"));
    jmw_demo=0;
    return;
  }
  if (_tcscmp(misc, TEXT("diagnostics"))==0) {
    VarioWriteNMEA(TEXT("PDVSC,S,Diagnostics,2"));
    jmw_demo=0;
    return;
  }
  if (_tcscmp(misc, TEXT("psraw"))==0) {
    VarioWriteNMEA(TEXT("PDVSC,S,Diagnostics,3"));
    return;
  }
  if (_tcscmp(misc, TEXT("switch"))==0) {
    VarioWriteNMEA(TEXT("PDVSC,S,Diagnostics,4"));
    return;
  }
  if (_tcscmp(misc, TEXT("democlimb"))==0) {
    VarioWriteNMEA(TEXT("PDVSC,S,DemoMode,0"));
    VarioWriteNMEA(TEXT("PDVSC,S,DemoMode,2"));
    jmw_demo=2;
    return;
  }
  if (_tcscmp(misc, TEXT("demostf"))==0) {
    VarioWriteNMEA(TEXT("PDVSC,S,DemoMode,0"));
    VarioWriteNMEA(TEXT("PDVSC,S,DemoMode,1"));
    jmw_demo=1;
    return;
  }
  if (_tcscmp(misc, TEXT("accel")) == 0) {
    switch(naccel) {
    case 0:
      VarioWriteNMEA(TEXT("PDVSC,R,AccelerometerSlopeX"));
      break;
    case 1:
      VarioWriteNMEA(TEXT("PDVSC,R,AccelerometerSlopeY"));
      break;
    case 2:
      VarioWriteNMEA(TEXT("PDVSC,R,AccelerometerOffsetX"));
      break;
    case 3:
      VarioWriteNMEA(TEXT("PDVSC,R,AccelerometerOffsetY"));
      break;
    default:
      naccel=0;
      break;
    }
    naccel++;
    if (naccel>3) {
      naccel=0;
    }
    return;
  }
  if (_tcscmp(misc, TEXT("xdemo")) == 0) {
    dlgVegaDemoShowModal();
    return;
  }
  if (_tcscmp(misc, TEXT("zero"))==0) {
    if (!CALCULATED_INFO.Flying) {
      VarioWriteNMEA(TEXT("PDVSC,S,ZeroASI,1"));
    }
    // zero, no mixing
    return;
  }
  if (_tcscmp(misc, TEXT("save"))==0) {
    VarioWriteNMEA(TEXT("PDVSC,S,StoreToEeprom,2"));
    return;
  }

  // accel calibration
  if (!CALCULATED_INFO.Flying) {
    if (_tcscmp(misc, TEXT("X1"))==0) {
      VarioWriteNMEA(TEXT("PDVSC,S,CalibrateAccel,1"));
      return;
    }
    if (_tcscmp(misc, TEXT("X2"))==0) {
      VarioWriteNMEA(TEXT("PDVSC,S,CalibrateAccel,2"));
      return;
    }
    if (_tcscmp(misc, TEXT("X3"))==0) {
      VarioWriteNMEA(TEXT("PDVSC,S,CalibrateAccel,3"));
      return;
    }
    if (_tcscmp(misc, TEXT("X4"))==0) {
      VarioWriteNMEA(TEXT("PDVSC,S,CalibrateAccel,4"));
      return;
    }
    if (_tcscmp(misc, TEXT("X5"))==0) {
      VarioWriteNMEA(TEXT("PDVSC,S,CalibrateAccel,5"));
      return;
    }
  }
}


// Adjust audio deadband of internal vario sounds
// +: increases deadband
// -: decreases deadband
void InputEvents::eventAudioDeadband(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("+"))) {
    SoundDeadband++;
  }
  if (_tcscmp(misc, TEXT("-"))) {
    SoundDeadband--;
  }
  SoundDeadband = min(40,max(SoundDeadband,0));
  /*
  VarioSound_SetVdead(SoundDeadband);
  */
  SaveSoundSettings(); // save to registry

  // TODO feature: send to vario if available

}

// AdjustWaypoint
// Adjusts the active waypoint of the task
//  next: selects the next waypoint, stops at final waypoint
//  previous: selects the previous waypoint, stops at start waypoint
//  nextwrap: selects the next waypoint, wrapping back to start after final
//  previouswrap: selects the previous waypoint, wrapping to final after start
void InputEvents::eventAdjustWaypoint(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("next")) == 0) {
    NextUpDown(1); // next
  } else if (_tcscmp(misc, TEXT("nextwrap")) == 0) {
    NextUpDown(2); // next - with wrap
  } else if (_tcscmp(misc, TEXT("previous")) == 0) {
    NextUpDown(-1); // previous
  } else if (_tcscmp(misc, TEXT("previouswrap")) == 0) {
    NextUpDown(-2); // previous with wrap
  }
}

// AbortTask
// Allows aborting and resuming of tasks
// abort: aborts the task if active
// resume: resumes the task if aborted
// toggle: toggles between abort and resume
// show: displays a status message showing the task abort status
void InputEvents::eventAbortTask(const TCHAR *misc) {
  mutexTaskData.Lock();
  if (_tcscmp(misc, TEXT("abort")) == 0)
    ResumeAbortTask(1);
  else if (_tcscmp(misc, TEXT("resume")) == 0)
    ResumeAbortTask(-1);
  else if (_tcscmp(misc, TEXT("show")) == 0) {
    if (TaskAborted)
      Message::AddMessage(TEXT("Task Aborted"));
    else if (TaskIsTemporary()) {
      Message::AddMessage(TEXT("Task Temporary"));
    } else {
      Message::AddMessage(TEXT("Task Resume"));
    }
  } else {
    ResumeAbortTask(0);
  }
  mutexTaskData.Unlock();
}

#include "Device/device.h"
#include "McReady.h"

// Bugs
// Adjusts the degradation of glider performance due to bugs
// up: increases the performance by 10%
// down: decreases the performance by 10%
// max: cleans the aircraft of bugs
// min: selects the worst performance (50%)
// show: shows the current bug degradation
void InputEvents::eventBugs(const TCHAR *misc) {
  double BUGS = GlidePolar::GetBugs();
  double oldBugs = BUGS;

  mutexComm.Lock(); // Must LockComm to prevent deadlock
  mutexFlightData.Lock();

  if (_tcscmp(misc, TEXT("up")) == 0) {
    BUGS = iround(BUGS*100+10) / 100.0;
  }
  if (_tcscmp(misc, TEXT("down")) == 0) {
    BUGS = iround(BUGS*100-10) / 100.0;
  }
  if (_tcscmp(misc, TEXT("max")) == 0) {
    BUGS= 1.0;
  }
  if (_tcscmp(misc, TEXT("min")) == 0) {
    BUGS= 0.0;
  }
  if (_tcscmp(misc, TEXT("show")) == 0) {
    TCHAR Temp[100];
    _stprintf(Temp,TEXT("%d"), iround(BUGS*100));
    Message::AddMessage(TEXT("Bugs Performance"), Temp);
  }
  if (BUGS != oldBugs) {
    BUGS= min(1.0,max(0.5,BUGS));
    GlidePolar::SetBugs(BUGS);
    GlidePolar::UpdatePolar(true);
  }
  mutexFlightData.Unlock();
  mutexComm.Unlock();
}

// Ballast
// Adjusts the ballast setting of the glider
// up: increases ballast by 10%
// down: decreases ballast by 10%
// max: selects 100% ballast
// min: selects 0% ballast
// show: displays a status message indicating the ballast percentage
void InputEvents::eventBallast(const TCHAR *misc) {
  double BALLAST = GlidePolar::GetBallast();
  double oldBallast= BALLAST;
  mutexComm.Lock(); // Must mutexComm.Lock to prevent deadlock
  mutexFlightData.Lock();
  if (_tcscmp(misc, TEXT("up")) == 0) {
    BALLAST = iround(BALLAST*100.0+10) / 100.0;
  }
  if (_tcscmp(misc, TEXT("down")) == 0) {
    BALLAST = iround(BALLAST*100.0-10) / 100.0;
  }
  if (_tcscmp(misc, TEXT("max")) == 0) {
    BALLAST= 1.0;
  }
  if (_tcscmp(misc, TEXT("min")) == 0) {
    BALLAST= 0.0;
  }
  if (_tcscmp(misc, TEXT("show")) == 0) {
    TCHAR Temp[100];
    _stprintf(Temp,TEXT("%d"),iround(BALLAST*100));
    Message::AddMessage(TEXT("Ballast %"), Temp);
  }
  if (BALLAST != oldBallast) {
    BALLAST=min(1.0,max(0.0,BALLAST));
    GlidePolar::SetBallast(BALLAST);
    GlidePolar::UpdatePolar(true);
  }
  mutexFlightData.Unlock();
  mutexComm.Unlock();
}

#include "Task.h"
#include "Logger.h"


void InputEvents::eventAutoLogger(const TCHAR *misc) {
  if (!DisableAutoLogger) {
    eventLogger(misc);
  }
}

// Logger
// Activates the internal IGC logger
//  start: starts the logger
// start ask: starts the logger after asking the user to confirm
// stop: stops the logger
// stop ask: stops the logger after asking the user to confirm
// toggle: toggles between on and off
// toggle ask: toggles between on and off, asking the user to confirm
// show: displays a status message indicating whether the logger is active
// nmea: turns on and off NMEA logging
// note: the text following the 'note' characters is added to the log file
void InputEvents::eventLogger(const TCHAR *misc) {
  // TODO feature: start logger without requiring feedback
  // start stop toggle addnote

  // This StartupStore code is to track down events leading to "missing logger file" bug
  TCHAR szMessage[MAX_PATH] = TEXT("\0");
  _tcsncpy(szMessage, TEXT("eventLogger: "),MAX_PATH);
  _tcsncat(szMessage, misc,MAX_PATH);
  _tcsncat(szMessage,TEXT("\r\n"),MAX_PATH);
  StartupStore(szMessage);

  if (_tcscmp(misc, TEXT("start ask")) == 0) {
    guiStartLogger();
    return;
  } else if (_tcscmp(misc, TEXT("start")) == 0) {
    guiStartLogger(true);
    return;
  } else if (_tcscmp(misc, TEXT("stop ask")) == 0) {
    guiStopLogger();
    return;
  } else if (_tcscmp(misc, TEXT("stop")) == 0) {
    guiStopLogger(true);
    return;
  } else if (_tcscmp(misc, TEXT("toggle ask")) == 0) {
    guiToggleLogger();
    return;
  } else if (_tcscmp(misc, TEXT("toggle")) == 0) {
    guiToggleLogger(true);
    return;
  } else if (_tcscmp(misc, TEXT("nmea")) == 0) {
    EnableLogNMEA = !EnableLogNMEA;
    if (EnableLogNMEA) {
      Message::AddMessage(TEXT("NMEA Log ON"));
    } else {
      Message::AddMessage(TEXT("NMEA Log OFF"));
    }
    return;
  } else if (_tcscmp(misc, TEXT("show")) == 0) {
    if (LoggerActive) {
      Message::AddMessage(TEXT("Logger ON"));
    } else {
      Message::AddMessage(TEXT("Logger OFF"));
    }
  } else if (_tcsncmp(misc, TEXT("note"), 4)==0) {
    // add note to logger file if available..
    LoggerNote(misc+4);
  }
}

// RepeatStatusMessage
// Repeats the last status message.  If pressed repeatedly, will
// repeat previous status messages
void InputEvents::eventRepeatStatusMessage(const TCHAR *misc) {
  (void)misc;
	// new interface
  // TODO enhancement: display only by type specified in misc field
  Message::Repeat(0);
}

// NearestAirspaceDetails
// Displays details of the nearest airspace to the aircraft in a
// status message.  This does nothing if there is no airspace within
// 100km of the aircraft.
// If the aircraft is within airspace, this displays the distance and bearing
// to the nearest exit to the airspace.


bool dlgAirspaceWarningIsEmpty(void);

void InputEvents::eventNearestAirspaceDetails(const TCHAR *misc) {
  (void)misc;
  double nearestdistance=0;
  double nearestbearing=0;
  int foundcircle = -1;
  int foundarea = -1;
  int i;
  bool inside = false;

  TCHAR szMessageBuffer[MAX_PATH];
  TCHAR szTitleBuffer[MAX_PATH];
  TCHAR text[MAX_PATH];

  if (!dlgAirspaceWarningIsEmpty()) {
    RequestAirspaceWarningForce = true;
    airspaceWarningEvent.trigger();
    return;
  }

  StartHourglassCursor();
  FindNearestAirspace(map_window,
		      GPS_INFO.Longitude, GPS_INFO.Latitude,
		      &nearestdistance, &nearestbearing,
		      &foundcircle, &foundarea);
  StopHourglassCursor();

  if ((foundcircle == -1)&&(foundarea == -1)) {
    // nothing to display!
    return;
  }

  if (foundcircle != -1) {
    i = foundcircle;

    dlgAirspaceDetails(i, -1);
    /*
    FormatWarningString(AirspaceCircle[i].Type , AirspaceCircle[i].Name ,
			AirspaceCircle[i].Base, AirspaceCircle[i].Top,
			szMessageBuffer, szTitleBuffer );
    */
  } else if (foundarea != -1) {

    i = foundarea;
    dlgAirspaceDetails(-1, i);

    /*
    FormatWarningString(AirspaceArea[i].Type , AirspaceArea[i].Name ,
			AirspaceArea[i].Base, AirspaceArea[i].Top,
			szMessageBuffer, szTitleBuffer );
    */
  }

  return; /// JMW testing only

  if (nearestdistance<0) {
    inside = true;
    nearestdistance = -nearestdistance;
  }

  TCHAR DistanceText[MAX_PATH];
  Units::FormatUserDistance(nearestdistance, DistanceText, 10);

  if (inside && (CALCULATED_INFO.NavAltitude <= AirspaceArea[i].Top.Altitude)
      && (CALCULATED_INFO.NavAltitude >= AirspaceArea[i].Base.Altitude)) {

    _stprintf(text,
              TEXT("Inside airspace: %s\r\n%s\r\nExit: %s\r\nBearing %d")
	      TEXT(DEG)TEXT("\r\n"),
              szTitleBuffer,
              szMessageBuffer,
              DistanceText,
              (int)nearestbearing);
  } else {
    _stprintf(text,
	      TEXT("Nearest airspace: %s\r\n%s\r\nDistance: %s\r\nBearing %d")
	      TEXT(DEG)TEXT("\r\n"),
	      szTitleBuffer,
	      szMessageBuffer,
	      DistanceText,
	      (int)nearestbearing);
  }

  // clear previous warning if any
  Message::Acknowledge(Message::MSG_AIRSPACE);

  // TODO code: No control via status data (ala DoStatusMEssage)
  // - can we change this?
  Message::Lock();
  Message::AddMessage(5000, Message::MSG_AIRSPACE, text);
  Message::Unlock();
}

// NearestWaypointDetails
// Displays the waypoint details dialog
//  aircraft: the waypoint nearest the aircraft
//  pan: the waypoint nearest to the pan cursor
void InputEvents::eventNearestWaypointDetails(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("aircraft")) == 0) {
    PopupNearestWaypointDetails(GPS_INFO.Longitude,
				GPS_INFO.Latitude,
				1.0e5, // big range..
				false);
  }
  if (_tcscmp(misc, TEXT("pan")) == 0) {
    PopupNearestWaypointDetails(GPS_INFO.Longitude,
				GPS_INFO.Latitude,
				1.0e5, // big range..
				true);
  }
}

// Null
// The null event does nothing.  This can be used to override
// default functionality
void InputEvents::eventNull(const TCHAR *misc) {
	(void)misc;
  // do nothing
}

// TaskLoad
// Loads the task of the specified filename
void InputEvents::eventTaskLoad(const TCHAR *misc) {
  TCHAR buffer[MAX_PATH];
  if (_tcslen(misc)>0) {
    mutexTaskData.Lock();
    LocalPath(buffer,misc);
    LoadNewTask(buffer);
    mutexTaskData.Unlock();
  }
}

// TaskSave
// Saves the task to the specified filename
void InputEvents::eventTaskSave(const TCHAR *misc) {
  TCHAR buffer[MAX_PATH];
  if (_tcslen(misc)>0) {
    mutexTaskData.Lock();
    LocalPath(buffer, misc);
    SaveTask(buffer);
    mutexTaskData.Unlock();
  }
}

// ProfileLoad
// Loads the profile of the specified filename
void InputEvents::eventProfileLoad(const TCHAR *misc) {
  if (_tcslen(misc)>0) {
    ReadProfile(misc);
  }
}

// ProfileSave
// Saves the profile to the specified filename
void InputEvents::eventProfileSave(const TCHAR *misc) {
  if (_tcslen(misc)>0) {
    WriteProfile(misc);
  }
}


void InputEvents::eventBeep(const TCHAR *misc) {
#ifndef DISABLEAUDIO
  MessageBeep(MB_ICONEXCLAMATION);
#endif
#if defined(GNAV)
  InputEvents::eventDLLExecute(TEXT("altairplatform.dll DoBeep2 1"));
#endif
}

void SystemConfiguration(void);

// Setup
// Activates configuration and setting dialogs
//  Basic: Basic settings (QNH/Bugs/Ballast/MaxTemperature)
//  Wind: Wind settings
//  Task: Task editor
//  Airspace: Airspace filter settings
//  Replay: IGC replay dialog
void InputEvents::eventSetup(const TCHAR *misc) {

  if (_tcscmp(misc,TEXT("Basic"))==0){
    dlgBasicSettingsShowModal();
  } else if (_tcscmp(misc,TEXT("Wind"))==0){
    dlgWindSettingsShowModal();
  } else if (_tcscmp(misc,TEXT("System"))==0){
    SystemConfiguration();
  } else if (_tcscmp(misc,TEXT("Task"))==0){
    dlgTaskOverviewShowModal();
  } else if (_tcscmp(misc,TEXT("Airspace"))==0){
    dlgAirspaceShowModal(false);
  } else if (_tcscmp(misc,TEXT("Weather"))==0){
    dlgWeatherShowModal();
  } else if (_tcscmp(misc,TEXT("Replay"))==0){
    if (!GPS_INFO.MovementDetected) {
      dlgLoggerReplayShowModal();
    }
  } else if (_tcscmp(misc,TEXT("Switches"))==0){
    dlgSwitchesShowModal();
  } else if (_tcscmp(misc,TEXT("Voice"))==0){
    dlgVoiceShowModal();
  } else if (_tcscmp(misc,TEXT("Teamcode"))==0){
    dlgTeamCodeShowModal();
  } else if (_tcscmp(misc,TEXT("Target"))==0){
    dlgTarget();
  }

}


// DLLExecute
// Runs the plugin of the specified filename
void InputEvents::eventDLLExecute(const TCHAR *misc) {
  // LoadLibrary(TEXT("test.dll"));

  StartupStore(TEXT("%s\n"), misc);

  TCHAR data[MAX_PATH];
  TCHAR* dll_name;
  TCHAR* func_name;
  TCHAR* other;
  TCHAR* pdest;

  _tcscpy(data, misc);

  // dll_name (up to first space)
  pdest = _tcsstr(data, TEXT(" "));
  if (pdest == NULL) {
#ifdef _INPUTDEBUG_
    _stprintf(input_errors[input_errors_count++],
              TEXT("Invalid DLLExecute string - no DLL"));
    InputEvents::showErrors();
#endif
    return;
  }
  *pdest = _T('\0');
  dll_name = data;

  // func_name (after first space)
  func_name = pdest + 1;

  // other (after next space to end of string)
  pdest = _tcsstr(func_name, TEXT(" "));
  if (pdest != NULL) {
    *pdest = _T('\0');
    other = pdest + 1;
  } else {
    other = NULL;
  }

  HINSTANCE hinstLib;	// Library pointer
  DLLFUNC_INPUTEVENT lpfnDLLProc = NULL;	// Function pointer

  // Load library, find function, execute, unload library
  hinstLib = _loadDLL(dll_name);
  if (hinstLib != NULL) {
#if !(defined(__MINGW32__)&&(WINDOWSPC>0))
    lpfnDLLProc = (DLLFUNC_INPUTEVENT)GetProcAddress(hinstLib, func_name);
#endif
    if (lpfnDLLProc != NULL) {
      (*lpfnDLLProc)(other);
#ifdef _INPUTDEBUG_
    } else {
      DWORD le;
      le = GetLastError();
      _stprintf(input_errors[input_errors_count++],
		TEXT("Problem loading function (%s) in DLL (%s) = %d"),
		func_name, dll_name, le);
      InputEvents::showErrors();
#endif
    }
  }
}

// Load a DLL (only once, keep a cache of the handle)
//	TODO code: FreeLibrary - it would be nice to call FreeLibrary
//      before exit on each of these
HINSTANCE _loadDLL(TCHAR *name) {
  int i;
  for (i = 0; i < DLLCache_Count; i++) {
    if (_tcscmp(name, DLLCache[i].text) == 0)
      return DLLCache[i].hinstance;
  }
  if (DLLCache_Count < MAX_DLL_CACHE) {
    DLLCache[DLLCache_Count].hinstance = LoadLibrary(name);
    if (DLLCache[DLLCache_Count].hinstance) {
      DLLCache[DLLCache_Count].text = StringMallocParse(name);
      DLLCache_Count++;

      // First time setup... (should check version numbers etc...)
      DLLFUNC_SETHINST lpfnDLLProc = NULL;
#if !(defined(__MINGW32__)&&(WINDOWSPC>0))
      lpfnDLLProc = (DLLFUNC_SETHINST)
	GetProcAddress(DLLCache[DLLCache_Count - 1].hinstance,
		       TEXT("XCSAPI_SetHInst"));
#endif
      if (lpfnDLLProc)
	lpfnDLLProc(GetModuleHandle(NULL));

      return DLLCache[DLLCache_Count - 1].hinstance;
#ifdef _INPUTDEBUG_
    } else {
      _stprintf(input_errors[input_errors_count++],
		TEXT("Invalid DLLExecute - not loaded - %s"), name);
      InputEvents::showErrors();
#endif
    }
  }

  return NULL;
}

// AdjustForecastTemperature
// Adjusts the maximum ground temperature used by the convection forecast
// +: increases temperature by one degree celsius
// -: decreases temperature by one degree celsius
// show: Shows a status message with the current forecast temperature
void InputEvents::eventAdjustForecastTemperature(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("+")) == 0) {
    CuSonde::adjustForecastTemperature(1.0);
  }
  if (_tcscmp(misc, TEXT("-")) == 0) {
    CuSonde::adjustForecastTemperature(-1.0);
  }
  if (_tcscmp(misc, TEXT("show")) == 0) {
    TCHAR Temp[100];
    _stprintf(Temp,TEXT("%f"),CuSonde::maxGroundTemperature);
    Message::AddMessage(TEXT("Forecast temperature"), Temp);
  }
}

// Run
// Runs an external program of the specified filename.
// Note that XCSoar will wait until this program exits.
void InputEvents::eventRun(const TCHAR *misc) {
  PROCESS_INFORMATION pi;
  if (!::CreateProcess(misc,
		       NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi))
    return;

  // wait for program to finish!
  ::WaitForSingleObject(pi.hProcess, INFINITE);
}


void InputEvents::eventDeclutterLabels(const TCHAR *misc) {
  if (_tcscmp(misc, TEXT("toggle")) == 0) {
    DeclutterLabels ++;
    DeclutterLabels = DeclutterLabels % 3;
  } else if (_tcscmp(misc, TEXT("on")) == 0)
    DeclutterLabels = 2;
  else if (_tcscmp(misc, TEXT("off")) == 0)
    DeclutterLabels = 0;
  else if (_tcscmp(misc, TEXT("mid")) == 0)
    DeclutterLabels = 1;
  else if (_tcscmp(misc, TEXT("show")) == 0) {
    if (DeclutterLabels==0)
      Message::AddMessage(TEXT("Map labels ON"));
    else if (DeclutterLabels==1)
      Message::AddMessage(TEXT("Map labels MID"));
    else
      Message::AddMessage(TEXT("Map labels OFF"));
  }
}




void InputEvents::eventBrightness(const TCHAR *misc) {
	(void)misc;
  dlgBrightnessShowModal();
}


void InputEvents::eventExit(const TCHAR *misc) {
  (void)misc;
  SignalShutdown(false);
}

void InputEvents::eventUserDisplayModeForce(const TCHAR *misc){

  if (_tcscmp(misc, TEXT("unforce")) == 0){
    UserForceDisplayMode = dmNone;
  }
  else if (_tcscmp(misc, TEXT("forceclimb")) == 0){
    UserForceDisplayMode = dmCircling;
  }
  else if (_tcscmp(misc, TEXT("forcecruise")) == 0){
    UserForceDisplayMode = dmCruise;
  }
  else if (_tcscmp(misc, TEXT("forcefinal")) == 0){
    UserForceDisplayMode = dmFinalGlide;
  }
  else if (_tcscmp(misc, TEXT("show")) == 0){
    Message::AddMessage(TEXT("Map labels ON"));
  }
}

void InputEvents::eventAirspaceDisplayMode(const TCHAR *misc){

  if (_tcscmp(misc, TEXT("all")) == 0){
    AltitudeMode = ALLON;
  }
  else if (_tcscmp(misc, TEXT("clip")) == 0){
    AltitudeMode = CLIP;
  }
  else if (_tcscmp(misc, TEXT("auto")) == 0){
    AltitudeMode = AUTO;
  }
  else if (_tcscmp(misc, TEXT("below")) == 0){
    AltitudeMode = ALLBELOW;
  }
  else if (_tcscmp(misc, TEXT("off")) == 0){
    AltitudeMode = ALLOFF;
  }
}


void InputEvents::eventAddWaypoint(const TCHAR *misc) {
  static int tmpWaypointNum = 0;
  WAYPOINT edit_waypoint;
  mutexTaskData.Lock();
  edit_waypoint.Latitude = GPS_INFO.Latitude;
  edit_waypoint.Longitude = GPS_INFO.Longitude;
  edit_waypoint.Altitude = CALCULATED_INFO.TerrainAlt;
  edit_waypoint.FileNum = 2; // don't put into file
  edit_waypoint.Flags = 0;
  if (_tcscmp(misc, TEXT("landable")) == 0) {
    edit_waypoint.Flags += LANDPOINT;
  }
  edit_waypoint.Comment[0] = 0;
  edit_waypoint.Name[0] = 0;
  edit_waypoint.Details = 0;
  edit_waypoint.Number = NumberOfWayPoints;

  WAYPOINT *new_waypoint = GrowWaypointList();
  if (new_waypoint) {
    tmpWaypointNum++;
    memcpy(new_waypoint,&edit_waypoint,sizeof(WAYPOINT));
    _stprintf(new_waypoint->Name,TEXT("_%d"), tmpWaypointNum);
    new_waypoint->Details= 0;
  }
  mutexTaskData.Unlock();
}



void InputEvents::eventOrientation(const TCHAR *misc){
  if (_tcscmp(misc, TEXT("northup")) == 0){
    DisplayOrientation = NORTHUP;
  }
  else if (_tcscmp(misc, TEXT("northcircle")) == 0){
    DisplayOrientation = NORTHCIRCLE;
  }
  else if (_tcscmp(misc, TEXT("trackcircle")) == 0){
    DisplayOrientation = TRACKCIRCLE;
  }
  else if (_tcscmp(misc, TEXT("trackup")) == 0){
    DisplayOrientation = TRACKUP;
  }
  else if (_tcscmp(misc, TEXT("northtrack")) == 0){
    DisplayOrientation = NORTHTRACK;
  }
}






// JMW TODO enhancement: have all inputevents return bool, indicating whether
// the button should after processing be hilit or not.
// this allows the buttons to indicate whether things are enabled/disabled
// SDP TODO enhancement: maybe instead do conditional processing ?
//     I like this idea; if one returns false, then don't execute the
//     remaining events.

// JMW TODO enhancement: make sure when we change things here we also set registry values...
// or maybe have special tag "save" which indicates it should be saved (notice that
// the wind adjustment uses this already, see in Process.cpp)

/* Recently done

eventTaskLoad		- Load tasks from a file (misc = filename)
eventTaskSave		- Save tasks to a file (misc = filename)
eventProfileLoad		- Load profile from a file (misc = filename)
eventProfileSave		- Save profile to a file (misc = filename)

*/

/* TODO feature: - new events

eventPanWaypoint		                - Set pan to a waypoint
- Waypoint could be "next", "first", "last", "previous", or named
- Note: wrong name - probably just part of eventPan
eventPressure		- Increase, Decrease, show, Set pressure value
eventDeclare			- (JMW separate from internal logger)
eventAirspaceDisplay	- all, below nnn, below me, auto nnn
eventAirspaceWarnings- on, off, time nn, ack nn
eventTerrain			- see map_window.Event_Terrain
eventCompass			- on, off, cruise on, crusie off, climb on, climb off
eventVario			- on, off // JMW what does this do?
eventOrientation		- north, track,  ???
eventTerrainRange	        - on, off (might be part of eventTerrain)
eventSounds			- Include Task and Modes sounds along with Vario
- Include master nn, deadband nn, netto trigger mph/kts/...

*/
