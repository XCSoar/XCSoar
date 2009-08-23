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

#if !defined(AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include "StdAfx.h"
#include "Defines.h"
#include "resource.h"
#include "Sizes.h"
#include "Units.h"
#include "Statistics.h"
#include "Appearance.hpp"
#include "Formatter/Base.hpp"
#include "GlideRatio.hpp"

class Trigger;
class MapWindow;

class FormatterDiffBearing: public InfoBoxFormatter {
 public:
  FormatterDiffBearing(const TCHAR *theformat):InfoBoxFormatter(theformat) {};

  virtual const TCHAR *Render(int *color);
};


void ProcessChar1 (char c);
void ProcessChar2 (char c);

extern void UnlockEventQueue();
extern void LockEventQueue();
extern void UnlockComm();
extern void LockComm();
extern void UnlockGraphicsData();
extern void LockGraphicsData();
extern void UnlockFlightData();
extern void LockFlightData();
extern void UnlockTaskData();
extern void LockTaskData();
extern void UnlockTerrainDataCalculations();
extern void LockTerrainDataCalculations();
extern void UnlockTerrainDataGraphics();
extern void LockTerrainDataGraphics();
extern void UnlockNavBox();
extern void LockNavBox();
extern void TriggerGPSUpdate();
extern void TriggerVarioUpdate();
extern Trigger drawTriggerEvent;

void FocusOnWindow(int i, bool selected);
void FullScreen();

extern bool Debounce();

extern void PopupWaypointDetails();
extern void PopupAnalysis();


typedef enum{
	ae15seconds=0,
	ae30seconds,
	ae60seconds,
	ae90seconds,
	ae2minutes,
	ae3minutes,
} AverEffTime_t;


#if defined(PNA) || defined(FIVV)
// VENTA-ADDON MODEL
typedef enum{
	apImPnaGeneric=0,
	apImPnaHp31x,
	apImPnaMedionP5,
	apImPnaMio,
	apImPnaNokia500,
	apImPnaPn6000,
}InfoBoxModelAppearance_t;
#endif

typedef enum{
	evgNormal=0,
	evgExtended,
} ExtendedVisualGlide_t;

typedef enum{
	vkDisabled=0,
	vkEnabled,
} VirtualKeys_t;


extern TCHAR XCSoar_Version[256];

// instance of main program
extern HINSTANCE hInst;

// windows
extern HWND hWndMainWindow;           // HWND Main Window
extern MapWindow hWndMapWindow;

extern bool csFlightDataInitialized;

extern Appearance_t Appearance;

// Specials
#ifdef FIVV
extern double GPSAltitudeOffset; 	// VENTA3
#endif
extern double QFEAltitudeOffset; // VENTA3
extern int OnAirSpace; // VENTA3 toggle DrawAirSpace
extern bool WasFlying; // used by auto QFE..
extern double LastFlipBoxTime; // used by XCSoar and Calculations
extern bool EnableAutoBacklight; // VENTA4
extern bool EnableAutoSoundVolume; // VENTA4
extern bool ExtendedVisualGlide;
extern bool VirtualKeys;
extern short ArrivalValue;
extern short AverEffTime;

extern ldrotary_s rotaryLD;

// user controls/parameters
extern double MACCREADY;
extern bool   AutoMacCready;
extern double SAFETYALTITUDEARRIVAL;
extern double SAFETYALTITUDEBREAKOFF;
extern double SAFETYALTITUDETERRAIN;
extern double SAFTEYSPEED;

extern int NettoSpeed;
extern bool EnableAuxiliaryInfo;
extern int debounceTimeout;

// statistics
extern Statistics flightstats;

#if (EXPERIMENTAL > 0)
extern BlueDialupSMS bsms;
#endif

typedef enum {psInitInProgress=0, psInitDone=1, psFirstDrawDone=2, psNormalOp=3} StartupState_t;
// 0: not started at all
// 1: everything is alive
// 2: done first draw
// 3: normal operation

extern StartupState_t ProgramStarted;

// ******************************************************************

#endif // !defined(AFX_XCSOAR_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
